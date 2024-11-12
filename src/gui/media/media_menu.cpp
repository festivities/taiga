/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include "media_menu.hpp"

#include <QDesktopServices>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <ranges>

#include "base/string.hpp"
#include "gui/main/main_window.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "media/anime_utils.hpp"

namespace gui {

MediaMenu::MediaMenu(QWidget* parent, const QList<Anime>& items, const QMap<int, ListEntry> entries,
                     QItemSelectionModel* selectionModel)
    : QMenu(parent), m_items(items), m_entries(entries), m_selectionModel(selectionModel) {
  setAttribute(Qt::WA_DeleteOnClose);
}

void MediaMenu::popup() {
  if (m_items.empty()) return;

  addMediaItems();
  addSeparator();
  addListItems();
  addSeparator();
  addLibraryItems();
  addSeparator();
  addTorrentsItems();
  addSeparator();
  addMetaItems();

  QMenu::popup(QCursor::pos());
}

bool MediaMenu::isBatch() const {
  return m_items.size() > 1;
}

bool MediaMenu::isInList() const {
  return m_entries.size() == m_items.size();
}

bool MediaMenu::isNowPlaying() const {
  return false;  // @TODO
}

void MediaMenu::addToList(const anime::list::Status status) const {
  QMessageBox::information(nullptr, "TODO",
                           u"Status: %1"_s.arg(formatListStatus(status)));  // @TODO
}

void MediaMenu::editEpisode() const {
  QSet<int> watchedEpisodes;
  int maxValue = anime::kMaxEpisodeCount;

  for (const auto& item : m_items) {
    if (const auto entry = getEntry(item.id)) watchedEpisodes.insert(entry->watched_episodes);
    if (item.episode_count > 0) maxValue = std::min(maxValue, item.episode_count);
  }

  const int initalValue = watchedEpisodes.size() == 1 ? watchedEpisodes.values().front() : 0;

  bool ok = false;
  const auto value = QInputDialog::getInt(parentWidget(), tr("Edit Episodes Watched"),
                                          tr("Enter a number:"), initalValue, 0, maxValue, 1, &ok);
  if (!ok) return;
  QMessageBox::information(nullptr, "TODO", QString::number(value));  // @TODO
}

void MediaMenu::editNotes() const {
  bool ok = false;
  const auto notes =
      QInputDialog::getMultiLineText(parentWidget(), tr("Edit Notes"), tr("Enter notes:"), "", &ok);
  if (!ok) return;
  QMessageBox::information(nullptr, "TODO", notes);  // @TODO
}

void MediaMenu::editStatus(const anime::list::Status status) const {
  QMessageBox::information(nullptr, "TODO",
                           u"Status: %1"_s.arg(formatListStatus(status)));  // @TODO
}

void MediaMenu::playEpisode(int number) const {
  QMessageBox::information(nullptr, "TODO", u"Episode: %1"_s.arg(number));  // @TODO
}

void MediaMenu::removeFromList() const {
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Icon::Question);
  msgBox.setText("Do you want to remove selected items from your list?");

  QList<QString> titles;
  for (const auto& item : m_items) {
    titles.push_back(u"<li>%1</li>"_s.arg(QString::fromStdString(item.titles.romaji)));
  }
  msgBox.setInformativeText(u"<ul>%1</ul>"_s.arg(titles.join("")));

  auto removeButton = msgBox.addButton(tr("Remove"), QMessageBox::ButtonRole::DestructiveRole);
  msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == reinterpret_cast<QAbstractButton*>(removeButton)) {
    // @TODO: Add to queue
  }
}

void MediaMenu::search() const {
  const auto& item = m_items.front();
  mainWindow()->setPage(MainWindowPage::Search);
  mainWindow()->searchBox()->setText(QString::fromStdString(item.titles.romaji));
}

void MediaMenu::searchAniDB() const {
  for (const auto& item : m_items) {
    QUrl url{"https://anidb.net/anime/"};
    url.setQuery({{"adb.search", QString::fromStdString(item.titles.romaji)}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchAniList() const {
  for (const auto& item : m_items) {
    QUrl url{"https://anilist.co/search/anime"};
    QUrlQuery query{{"search", QString::fromStdString(item.titles.romaji)}};
    if (anime::isNsfw(item)) query.addQueryItem("adult", "true");
    url.setQuery(query);
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchANN() const {
  for (const auto& item : m_items) {
    QUrl url{"https://www.animenewsnetwork.com/search"};
    url.setQuery({{"q", QString::fromStdString(item.titles.romaji)}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchKitsu() const {
  for (const auto& item : m_items) {
    QUrl url{"https://kitsu.app/anime"};
    url.setQuery({{"text", QString::fromStdString(item.titles.romaji)}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchMyAnimeList() const {
  for (const auto& item : m_items) {
    QUrl url{"https://myanimelist.net/anime.php"};
    url.setQuery({{"q", QString::fromStdString(item.titles.romaji)}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchReddit() const {
  for (const auto& item : m_items) {
    QUrl url{"https://www.reddit.com/search"};
    const auto title = QString::fromStdString(item.titles.romaji);
    url.setQuery({
        {"q", u"subreddit:anime title:%1 episode discussion"_s.arg(title)},
        {"sort", "new"},
    });
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchWikipedia() const {
  for (const auto& item : m_items) {
    QUrl url{"https://en.wikipedia.org/wiki/Special:Search"};
    url.setQuery({{"search", QString::fromStdString(item.titles.romaji)}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchYouTube() const {
  for (const auto& item : m_items) {
    if (!item.trailer_id.empty()) {
      QUrl url{u"https://youtu.be/%1"_s.arg(QString::fromStdString(item.trailer_id))};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://www.youtube.com/results"};
      url.setQuery({{"search_query", QString::fromStdString(item.titles.romaji)}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::test() const {
  const auto action = reinterpret_cast<QAction*>(QObject::sender())->text();

  QList<QString> titles;
  for (const auto& item : m_items) {
    titles.push_back(QString::fromStdString(item.titles.romaji));
  }

  const auto text = u"Action: %1\n\n%2"_s.arg(action).arg(titles.join("\n"));

  QMessageBox::information(nullptr, "TODO", text);
}

void MediaMenu::viewDetails() const {
  if (m_items.empty()) return;

  const auto& anime = m_items.front();
  const auto entry = getEntry(anime.id);

  MediaDialog::show(parentWidget(), MediaDialogPage::Details, anime,
                    entry ? std::optional<ListEntry>{*entry} : std::nullopt);
}

void MediaMenu::edit() const {
  if (m_items.empty()) return;

  const auto& anime = m_items.front();
  const auto entry = getEntry(anime.id);

  MediaDialog::show(parentWidget(), MediaDialogPage::List, anime,
                    entry ? std::optional<ListEntry>{*entry} : std::nullopt);
}

void MediaMenu::addMediaItems() {
  if (!isBatch()) {
    // View details
    addAction(theme.getIcon("info"), tr("Details"), tr("Enter"), this, &MediaMenu::viewDetails);
    // Search
    addAction(theme.getIcon("search"), tr("Search"), this, &MediaMenu::search);
  }

  // External
  addMenu([this]() {
    auto menu = new QMenu(tr("External"), this);
    menu->setIcon(theme.getIcon("open_in_new"));

    using slot_t = void (MediaMenu::*)() const;
    const QList<QPair<QString, slot_t>> items = {
        {"AniDB", &MediaMenu::searchAniDB},
        {"AniList", &MediaMenu::searchAniList},
        {"Anime News Network", &MediaMenu::searchANN},
        {"Kitsu", &MediaMenu::searchKitsu},
        {"MyAnimeList", &MediaMenu::searchMyAnimeList},
        {"Reddit", &MediaMenu::searchReddit},
        {"Wikipedia", &MediaMenu::searchWikipedia},
        {"YouTube", &MediaMenu::searchYouTube},
    };
    for (const auto [text, slot] : items) {
      menu->addAction(text, this, slot);
    }

    return menu;
  }());
}

void MediaMenu::addListItems() {
  if (!isInList()) {
    // Add to list
    addMenu([this]() {
      auto menu = new QMenu(tr("Add to list"), this);
      menu->setIcon(theme.getIcon("list_alt"));
      for (const auto& status : anime::list::kStatuses) {
        menu->addAction(formatListStatus(status), this, [this, status]() { addToList(status); });
      }
      return menu;
    }());

    return;
  }

  // Edit
  if (!isBatch()) {
    addAction(theme.getIcon("edit"), tr("Edit..."), this, &MediaMenu::edit);

  } else {
    addMenu([this]() {
      auto menu = new QMenu(tr("Edit"), this);
      menu->setIcon(theme.getIcon("edit"));

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Date started"), this);
        menu->addAction(tr("Clear"), this, &MediaMenu::test);
        menu->addAction(tr("Set to date started airing"), this, &MediaMenu::test);
        return menu;
      }());

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Date completed"), this);
        menu->addAction(tr("Clear"), this, &MediaMenu::test);
        menu->addAction(tr("Set to date finished airing"), this, &MediaMenu::test);
        menu->addAction(tr("Set to last updated"), this, &MediaMenu::test);
        return menu;
      }());

      menu->addAction(tr("Episode..."), this, &MediaMenu::editEpisode);
      menu->addAction(tr("Notes..."), this, &MediaMenu::editNotes);

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Score"), this);
        for (int i = 0; i <= 10; ++i) {
          menu->addAction(tr("%1").arg(i), this, &MediaMenu::test);
        }
        return menu;
      }());

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Status"), this);
        for (const auto status : anime::list::kStatuses) {
          auto action = new QAction(formatListStatus(status), this);
          menu->addAction(action);
          connect(action, &QAction::triggered, this, [this, status]() { editStatus(status); });
        }
        return menu;
      }());

      return menu;
    }());
  }

  // Remove from list
  addAction(theme.getIcon("delete"), tr("Remove..."), QKeySequence::Delete, this,
            &MediaMenu::removeFromList);
}

void MediaMenu::addLibraryItems() {
  // Open folder
  addAction(theme.getIcon("folder"), tr("Open folder"), this, &MediaMenu::test);

  if (isBatch()) return;

  const auto& item = m_items.front();
  const auto entry = getEntry(item.id);

  // Play
  addMenu([this, &item, entry]() {
    const int total_episodes = item.episode_count;
    const int last_episode = entry ? std::min(entry->watched_episodes, total_episodes) : 0;
    const int next_episode = last_episode + 1;

    auto menu = new QMenu(tr("Play"), this);
    menu->setIcon(theme.getIcon("play_arrow"));

    // Play next episode
    if (next_episode < total_episodes || total_episodes == 1) {
      menu->addAction(theme.getIcon("skip_next"), tr("Next episode (#%1)").arg(next_episode), this,
                      [this, next_episode]() { playEpisode(next_episode); });
    }

    // Play last episode
    if (last_episode > 0) {
      menu->addAction(tr("Last episode (#%1)").arg(last_episode), this,
                      [this, last_episode]() { playEpisode(last_episode); });
    }

    if (total_episodes > 1) {
      // Play random episode
      menu->addAction(theme.getIcon("shuffle"), tr("Random episode"), this, [this]() {
        const int number = 3;  // @TODO
        playEpisode(number);
      });

      // Play episode
      menu->addSeparator();
      menu->addMenu([this, total_episodes, last_episode]() {
        auto menu = new QMenu(tr("Episode"), this);
        for (int i = 1; i <= total_episodes; ++i) {
          auto action = new QAction(u"#%1"_s.arg(i), this);
          action->setCheckable(true);
          action->setChecked(i <= last_episode);
          menu->addAction(action);
          connect(action, &QAction::triggered, this, [this, i]() { playEpisode(i); });
        }
        return menu;
      }());
    }

    // @TODO: Start new rewatch

    return menu;
  }());
}

void MediaMenu::addTorrentsItems() {
  if (isBatch()) return;

  // Torrents
  addAction(theme.getIcon("rss_feed"), tr("Torrents"), this, &MediaMenu::test);
}

void MediaMenu::addMetaItems() {
  if (isBatch() && m_selectionModel) {
    addAction(tr("Invert selection"), this, [this]() {
      for (int row = 0; row < m_selectionModel->model()->rowCount(); ++row) {
        const auto index = m_selectionModel->model()->index(row, 0);
        m_selectionModel->select(index, QItemSelectionModel::Toggle);
      }
    });
  }

  if (isNowPlaying() && !isBatch()) {
    addAction(tr("Set as now playing..."), this, &MediaMenu::test);
  }
}

const ListEntry* MediaMenu::getEntry(int id) const {
  const auto it = m_entries.find(id);
  return it != m_entries.end() ? &*it : nullptr;
}

}  // namespace gui
