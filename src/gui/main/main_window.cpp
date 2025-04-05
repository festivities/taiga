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

#include "main_window.hpp"

#include <utf8proc.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QtWidgets>

#include "base/string.hpp"
#include "gui/library/library_widget.hpp"
#include "gui/list/list_widget.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/now_playing_widget.hpp"
#include "gui/search/search_widget.hpp"
#include "gui/settings/settings_dialog.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/tray_icon.hpp"
#include "gui/utils/widgets.hpp"
#include "sync/service.hpp"
#include "taiga/application.hpp"
#include "taiga/session.hpp"
#include "taiga/version.hpp"
#include "ui_main_window.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

MainWindow::MainWindow() : QMainWindow(), ui_(new Ui::MainWindow) {
  ui_->setupUi(this);

  ui_->menubar->hide();

#ifdef Q_OS_WINDOWS
  enableMicaBackground(this);
#endif

  if (const auto geometry = taiga::session.mainWindowGeometry(); !geometry.isEmpty()) {
    restoreGeometry(geometry);
    centerWidgetToScreen(this);
  }

  // Do not call `init()` here, as it relies on the main window pointer being
  // available through the application instance.
}

MainWindow* mainWindow() {
  return taiga::app()->mainWindow();
}

NavigationWidget* MainWindow::navigation() const {
  return m_navigationWidget;
}

NowPlayingWidget* MainWindow::nowPlaying() const {
  return m_nowPlayingWidget;
}

QLineEdit* MainWindow::searchBox() const {
  return m_searchBox;
}

Ui::MainWindow* MainWindow::ui() const {
  return ui_;
}

void MainWindow::init() {
  initActions();
  initIcons();
  initTrayIcon();
  initToolbar();
  initNavigation();
  initStatusbar();
  initNowPlaying();
  updateTitle();
}

void MainWindow::initActions() {
  ui_->actionProfile->setToolTip(tr("Profile"));
  ui_->actionSynchronize->setToolTip(
      tr("Synchronize with %1").arg(sync::serviceName(sync::currentServiceId())));

  connect(ui_->actionAddNewFolder, &QAction::triggered, this, &MainWindow::addNewFolder);
  connect(ui_->actionExit, &QAction::triggered, this, &QApplication::quit, Qt::QueuedConnection);
  connect(ui_->actionSettings, &QAction::triggered, this, [this]() { SettingsDialog::show(this); });
  connect(ui_->actionAbout, &QAction::triggered, this, &MainWindow::about);
  connect(ui_->actionDonate, &QAction::triggered, this, &MainWindow::donate);
  connect(ui_->actionSupport, &QAction::triggered, this, &MainWindow::support);
  connect(ui_->actionProfile, &QAction::triggered, this, &MainWindow::profile);
  connect(ui_->actionDisplayWindow, &QAction::triggered, this, &MainWindow::displayWindow);

  connect(ui_->actionSynchronize, &QAction::triggered, this, [this]() {
    setEnabled(false);
    statusBar()->showMessage(
        tr("Synchronizing with %1...").arg(sync::serviceName(sync::currentServiceId())));
    QEventLoop loop;
    QTimer::singleShot(3000, &loop, [this, &loop]() {
      setEnabled(true);
      statusBar()->clearMessage();
      loop.quit();
    });
    loop.exec();
  });
}

void MainWindow::initIcons() {
  ui_->menuLibraryFolders->setIcon(theme.getIcon("folder"));
  ui_->menuExport->setIcon(theme.getIcon("export_notes"));

  ui_->actionAddNewFolder->setIcon(theme.getIcon("create_new_folder"));
  ui_->actionAbout->setIcon(theme.getIcon("info"));
  ui_->actionBack->setIcon(theme.getIcon("arrow_back"));
  ui_->actionCheckForUpdates->setIcon(theme.getIcon("cloud_download"));
  ui_->actionDonate->setIcon(theme.getIcon("favorite"));
  ui_->actionExit->setIcon(theme.getIcon("logout"));
  ui_->actionForward->setIcon(theme.getIcon("arrow_forward"));
  ui_->actionLibraryFolders->setIcon(theme.getIcon("folder"));
  ui_->actionMenu->setIcon(theme.getIcon("menu"));
  ui_->actionPlayNextEpisode->setIcon(theme.getIcon("skip_next"));
  ui_->actionPlayRandomAnime->setIcon(theme.getIcon("shuffle"));
  ui_->actionProfile->setIcon(theme.getIcon("account_circle"));
  ui_->actionScanAvailableEpisodes->setIcon(theme.getIcon("pageview"));
  ui_->actionSettings->setIcon(theme.getIcon("settings"));
  ui_->actionSupport->setIcon(theme.getIcon("help"));
  ui_->actionSynchronize->setIcon(theme.getIcon("sync"));
}

void MainWindow::initNavigation() {
  m_navigationWidget = new NavigationWidget(this);

  connect(m_navigationWidget, &NavigationWidget::currentPageChanged, this, &MainWindow::setPage);

  if (const auto item = m_navigationWidget->findItemByPage(MainWindowPage::List)) {
    m_navigationWidget->setCurrentItem(item);
  }

  ui_->splitter->insertWidget(0, m_navigationWidget);
}

void MainWindow::initNowPlaying() {
  m_nowPlayingWidget = new NowPlayingWidget(ui_->centralWidget);

  ui_->centralWidget->layout()->addWidget(m_nowPlayingWidget);
  m_nowPlayingWidget->hide();
}

void MainWindow::initPage(MainWindowPage page) {
  static QSet<MainWindowPage> initializedPages;

  if (initializedPages.contains(page)) return;

  static const auto init_page = [](QWidget* page, QWidget* widget) {
    const auto layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    page->setLayout(layout);
  };

  switch (page) {
    case MainWindowPage::Home:
      break;

    case MainWindowPage::Search:
      m_searchWidget = new SearchWidget(ui_->searchPage);
      init_page(ui_->searchPage, m_searchWidget);
      break;

    case MainWindowPage::List:
      m_listWidget = new ListWidget(ui_->listPage);
      init_page(ui_->listPage, m_listWidget);
      break;

    case MainWindowPage::History:
      break;

    case MainWindowPage::Library:
      m_libraryWidget = new LibraryWidget(ui_->libraryPage);
      init_page(ui_->libraryPage, m_libraryWidget);
      break;

    case MainWindowPage::Torrents:
      break;

    case MainWindowPage::Profile:
      break;
  }

  initializedPages.insert(page);
}

void MainWindow::initStatusbar() {
  ui_->statusbar->setContentsMargins(0, 8, 0, 0);

  ui_->statusbar->showMessage(tr("How are you today?"), 5000);
}

void MainWindow::initToolbar() {
  ui_->toolbar->setIconSize(QSize{24, 24});

  // Menu
  {
    const auto button = static_cast<QToolButton*>(ui_->toolbar->widgetForAction(ui_->actionMenu));
    button->setPopupMode(QToolButton::InstantPopup);
    button->setMenu([this]() {
      auto menu = new QMenu(this);
      menu->addAction(ui_->actionToggleDetection);
      menu->addAction(ui_->actionToggleSharing);
      menu->addAction(ui_->actionToggleSynchronization);
      menu->addSeparator();
      menu->addMenu(ui_->menuHelp);
      menu->addSeparator();
      menu->addAction(ui_->actionExit);
      return menu;
    }());
  }

  // Search box
  {
    m_searchBox = new QLineEdit();
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setFixedWidth(320);
    m_searchBox->setPlaceholderText(tr("Search"));

    const auto before = ui_->actionSettings;
    const auto insertSpacer = [this](QAction* before) {
      ui_->toolbar->insertWidget(before, [this]() {
        auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return spacer;
      }());
    };

    insertSpacer(before);
    ui_->toolbar->insertWidget(before, m_searchBox);
    insertSpacer(before);
  }
}

void MainWindow::initTrayIcon() {
  auto menu = new QMenu(this);
  menu->addAction(ui_->actionDisplayWindow);
  menu->setDefaultAction(ui_->actionDisplayWindow);
  menu->addSeparator();
  menu->addAction(ui_->actionSettings);
  menu->addSeparator();
  menu->addAction(ui_->actionExit);

  m_trayIcon = new TrayIcon(this, windowIcon(), menu);

  connect(m_trayIcon, &TrayIcon::activated, this, &MainWindow::displayWindow);
  connect(m_trayIcon, &TrayIcon::messageClicked, this,
          []() { QMessageBox::information(nullptr, "Taiga", tr("Clicked message")); });
}

void MainWindow::closeEvent(QCloseEvent* event) {
  taiga::session.setMainWindowGeometry(saveGeometry());
  if (m_listWidget) m_listWidget->saveState();
  if (m_searchWidget) m_searchWidget->saveState();
  event->accept();
}

void MainWindow::addNewFolder() {
  constexpr auto options =
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly;

  const auto directory = QFileDialog::getExistingDirectory(this, tr("Add New Folder"), "", options);

  if (!directory.isEmpty()) {
    QMessageBox::information(this, "New Folder", directory);
  }
}

void MainWindow::setPage(MainWindowPage page) {
  initPage(page);
  ui_->statusbar->clearMessage();
  ui_->stackedWidget->setCurrentIndex(static_cast<int>(page));
}

void MainWindow::updateTitle() {
  auto title = u"Taiga"_s;

  const auto app = static_cast<taiga::Application*>(qApp);

  if (app->isDebug()) {
    title += u" [debug]"_s;
  }

  setWindowTitle(title);
}

void MainWindow::displayWindow() {
  setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  activateWindow();
}

void MainWindow::about() {
  const auto version = QString::fromStdString(taiga::version().to_string());

  const auto contributors =
      "saka, Diablofan, slevir, LordGravewish, rr-, sunjayc, ConnorKrammer, Soinou, Jiyuu, ryban, "
      "tollyx, pavelxdd, gunt3001, synthtech, cnguy, CeruleanSky, Xabis, rzumer, Juplay, "
      "SacredZenpie";

  const auto donators =
      "Farfie, snickler, Nydaleclya, WizardTim, Kinzer, MeGaNeKo, WhatsCPS, Jerico64, Xen the Greedy";

  const auto components =
      u"Material Symbols, Qt %1, utf8proc %2"_s.arg(QT_VERSION_STR).arg(utf8proc_version());

  const QStringList lines{
      u"<big><b>Taiga</b> %1</big>"_s.arg(version),
      tr("<a href='%1'>Website</a> · "
         "<a href='%2'>GitHub</a> · "
         "<a href='%3'>Discord</a>")
          .arg("https:/taiga.moe/")
          .arg("https://github.com/erengy/taiga")
          .arg("https://discord.gg/yeGNktZ"),
      u"<b>%1:</b><br>erengy (Eren Okka)"_s.arg(tr("Author")),
      u"<b>%1:</b><br>%2"_s.arg(tr("Contributors")).arg(contributors),
      u"<b>%1:</b><br>%2 %3"_s.arg(tr("Donators"))
          .arg(donators)
          .arg(tr("and other anonymous supporters")),
      u"<b>%1:</b><br>%2"_s.arg(tr("Third-party components")).arg(components),
  };

  QMessageBox::about(this, tr("About Taiga"), lines.join("<br><br>"));
}

void MainWindow::donate() const {
  QDesktopServices::openUrl(QUrl("https://taiga.moe/#donate"));
}

void MainWindow::support() const {
  QDesktopServices::openUrl(QUrl("https://taiga.moe/#support"));
}

void MainWindow::profile() {
  setPage(MainWindowPage::Profile);
  m_navigationWidget->setCurrentIndex({});
}

}  // namespace gui
