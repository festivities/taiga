/**
 * Taiga
 * Copyright (C) 2010-2025, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "recognition.hpp"

#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <anitomy.hpp>
#include <ranges>
#include <vector>

#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "track/episode.hpp"
#include "track/recognition_cache.hpp"
#include "track/recognition_normalize.hpp"

namespace track::recognition {

Episode parse(std::string_view input, const anitomy::Options options) {
  Episode episode;

  auto elements = anitomy::parse(input, options);
  episode.setElements(elements);

  return episode;
}

Episode parseFileInfo(const QFileInfo& info, const anitomy::Options options) {
  const auto fileName = info.fileName().toStdString();

  Episode episode = track::recognition::parse(fileName, options);

  if (!episode.contains(anitomy::ElementKind::Title)) {
    const auto dirName = info.dir().dirName().toStdString();
    episode.addElement(anitomy::ElementKind::Title, dirName);
  }

  return episode;
}

int identify(Episode& episode) {
  cache()->init();

  const auto title = episode.element(anitomy::ElementKind::Title);
  const auto normalizedTitle = normalize(title);

  std::vector<Cache::Data::Match> matches;

  if (const auto data = cache()->find(normalizedTitle)) {
    matches.append_range(data->matches | std::views::values | std::ranges::to<std::vector>());
  }

  std::ranges::sort(matches, {}, &Cache::Data::Match::score);

  for (const auto& match : matches) {
    if (isValidMatch(match.id, episode)) return match.id;
  }

  return anime::kUnknownId;
}

bool isValidMatch(const int id, const Episode& episode) {
  const auto item = anime::db.item(id);

  if (!item) return false;

  const auto is_valid_episode_number = [&episode, &item]() {
    const auto number = episode.element(anitomy::ElementKind::Episode);

    if (number.empty()) {
      if (item->episode_count == 1)
        return true;  // single-episode anime can do without an episode number

      const auto extension = episode.element(anitomy::ElementKind::FileExtension);
      if (extension.empty()) return true;  // batch release
    }

    const int value = QString::fromStdString(number).toInt();
    if (value <= item->episode_count) return true;  // in range

    if (item->episode_count < 1) return true;  // episode count is unknown, so anything goes

    return false;  // out of range
  };

  if (!is_valid_episode_number()) return false;

  return true;
}

}  // namespace track::recognition
