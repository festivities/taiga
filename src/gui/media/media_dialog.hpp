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

#pragma once

#include <QDialog>
#include <optional>

#include "media/anime.hpp"
#include "media/anime_list.hpp"

class QResizeEvent;
class QShowEvent;

namespace Ui {
class MediaDialog;
}

namespace gui {

enum class MediaDialogPage {
  Details,
  List,
  Settings,
};

class MediaDialog final : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MediaDialog)

public:
  MediaDialog(QWidget* parent);
  ~MediaDialog() = default;

  static void show(QWidget* parent, MediaDialogPage page, const Anime& anime,
                   const std::optional<ListEntry> entry);

public slots:
  void accept() override;
  void setAnime(const Anime& anime, const std::optional<ListEntry> entry);

protected:
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;

private:
  void initTitles();
  void initDetails();
  void initList();
  void loadPosterImage();
  void resizePosterImage();

  Ui::MediaDialog* ui_ = nullptr;

  Anime m_anime;
  std::optional<ListEntry> m_entry;
};

}  // namespace gui
