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

#include "anime_list_item_delegate_cards.hpp"

#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>

#include "base/string.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/painter_state_saver.hpp"
#include "gui/utils/painters.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

constexpr int itemHeight = 210;
constexpr int posterWidth = itemHeight * 2 / 3;

ListItemDelegateCards::ListItemDelegateCards(QObject* parent) : QStyledItemDelegate(parent) {}

void ListItemDelegateCards::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
  const PainterStateSaver painterStateSaver(painter);

  const auto font = painter->font();

  const auto item =
      index.data(static_cast<int>(AnimeListItemDataRole::Anime)).value<const Anime*>();
  const auto entry =
      index.data(static_cast<int>(AnimeListItemDataRole::ListEntry)).value<const ListEntry*>();

  QStyleOptionViewItem opt = option;
  QRect rect = opt.rect;

  QPainterPath path;
  path.addRoundedRect(rect, 4, 4);
  painter->setClipPath(path);

  // Background
  if (option.state & QStyle::State_Selected) {
    painter->fillRect(rect, opt.palette.highlight());
  } else if (theme.isDark()) {
    painter->fillRect(rect, opt.palette.mid());
  } else {
    painter->fillRect(rect, opt.palette.alternateBase());
  }

  // Poster
  {
    QRect posterRect = rect;
    posterRect.setWidth(posterWidth);

    if (theme.isDark()) {
      painter->fillRect(posterRect, opt.palette.dark());
    } else {
      painter->fillRect(posterRect, opt.palette.mid());
    }

    const auto pixmap =
        index.data(static_cast<int>(AnimeListItemDataRole::Poster)).value<const QPixmap*>();

    if (!pixmap->isNull()) {
      const auto scaled =
          pixmap->size().scaled(posterRect.size(), Qt::AspectRatioMode::KeepAspectRatioByExpanding);

      QRect sourceRect{pixmap->rect()};
      if (scaled.width() > posterRect.width()) {
        const auto half = (scaled.width() - posterRect.width()) / 2.0f;
        const auto scale = static_cast<float>(pixmap->width()) / scaled.width();
        sourceRect.adjust(half * scale, 0, -half * scale, 0);
      } else {
        const auto half = (scaled.height() - posterRect.height()) / 2.0f;
        const auto scale = static_cast<float>(pixmap->height()) / scaled.height();
        sourceRect.adjust(0, half * scale, 0, -half * scale);
      }

      painter->drawPixmap(posterRect, *pixmap, sourceRect);
    }
  }

  if (entry) {
    auto progressOptions = opt;
    progressOptions.rect = rect;
    progressOptions.rect.setWidth(posterWidth);
    progressOptions.rect.setTop(progressOptions.rect.bottom() - 28);
    progressOptions.rect.adjust(4, 4, -4, -4);
    paintProgressBar(painter, progressOptions, item, entry);
  }

  rect.adjust(posterWidth, 0, 0, 0);

  // Title
  {
    QRect titleRect = rect;
    titleRect.setHeight(24);

    painter->fillRect(titleRect, opt.palette.dark());
    titleRect.adjust(8, 0, -8, 0);

    auto titleFont = painter->font();
    titleFont.setWeight(QFont::Weight::DemiBold);
    painter->setFont(titleFont);

    const QString title = index.data(Qt::DisplayRole).toString();
    const QFontMetrics metrics(painter->font());
    const QString elidedTitle = metrics.elidedText(title, Qt::ElideRight, titleRect.width());

    painter->drawText(titleRect, Qt::AlignVCenter | Qt::TextSingleLine, elidedTitle);

    rect.adjust(8, 24 + 8, -8, -8);
  }

  // Summary
  {
    QStringList parts{formatType(item->type), formatScore(item->score)};
    if (item->episode_count != 1) {
      parts.insert(1, tr("%1 episodes").arg(formatNumber(item->episode_count, "?")));
    }
    const QString summary = parts.join(" · ");
    const QFontMetrics metrics(painter->font());
    QRect summaryRect = rect;
    summaryRect.setHeight(metrics.height());
    painter->setFont(font);
    painter->drawText(summaryRect, Qt::AlignVCenter | Qt::TextSingleLine, summary);
    rect.adjust(0, summaryRect.height() + 8, 0, 0);
  }

  auto detailsFont = painter->font();

  // Details
  {
    const QString titles =
        "Aired:\n"
        "Genres:\n"
        "Studios:";
    const QString values =
        u"%1 (%2)\n%3\n%4"_s.arg(formatFuzzyDateRange(item->date_started, item->date_finished))
            .arg(formatStatus(item->status))
            .arg(joinStrings(item->genres))
            .arg(joinStrings(item->studios));

    detailsFont.setWeight(QFont::Weight::DemiBold);
    painter->setFont(detailsFont);

    const QFontMetrics metrics(painter->font());

    QRect titlesRect = rect;
    titlesRect.setHeight(metrics.height() * 3);
    titlesRect.setWidth(metrics.boundingRect("Studios:").width());

    painter->drawText(titlesRect, 0, titles);

    detailsFont.setWeight(QFont::Weight::Normal);
    painter->setFont(detailsFont);

    QRect valuesRect = rect;
    valuesRect.setHeight(metrics.height() * 3);
    valuesRect.adjust(titlesRect.width() + 8, 0, 0, 0);

    painter->drawText(valuesRect, 0, values);

    rect.adjust(0, titlesRect.height() + 8, 0, 0);
  }

  // Synopsis
  {
    QString synopsis = QString::fromStdString(item->synopsis);
    synopsis.replace("<br>", "\n");
    removeHtmlTags(synopsis);
    synopsis = synopsis.simplified();

    painter->setPen(opt.palette.placeholderText().color());

    detailsFont.setPointSize(8);
    painter->setFont(detailsFont);
    const QFontMetrics metrics(painter->font());

    QRect synopsisRect = rect;
    synopsisRect.setHeight(qMin(synopsisRect.height(), metrics.height() * 5));

    painter->drawText(synopsisRect, Qt::TextWordWrap, synopsis);
  }
}

QSize ListItemDelegateCards::sizeHint(const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
  if (index.isValid()) return itemSize();
  return QStyledItemDelegate::sizeHint(option, index);
}

void ListItemDelegateCards::initStyleOption(QStyleOptionViewItem* option,
                                            const QModelIndex& index) const {
  QStyledItemDelegate::initStyleOption(option, index);

  option->features &= ~QStyleOptionViewItem::ViewItemFeature::HasDisplay;
  option->features &= ~QStyleOptionViewItem::ViewItemFeature::HasDecoration;
}

QSize ListItemDelegateCards::itemSize() const {
  constexpr int maxColumns = 4;
  constexpr int maxItemWidth = 360;

  const auto parent = reinterpret_cast<QListView*>(this->parent());
  const int spacing = parent->spacing();

  const int availableWidth =
      parent->geometry().width() - ((2 * spacing) + parent->verticalScrollBar()->width());

  const int columns = [&]() {
    for (int i = maxColumns; i >= 1; --i) {
      if (availableWidth - (i * spacing) > i * maxItemWidth) return i;
    }
    return 1;
  }();

  const int columnsWidth = availableWidth - (columns * spacing);
  const float itemWidth = columnsWidth / static_cast<float>(columns);

  return QSize(std::floor(itemWidth), itemHeight);
}

}  // namespace gui
