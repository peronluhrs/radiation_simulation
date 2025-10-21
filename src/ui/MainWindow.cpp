#include "visualization/View3D.h"
#include "ui/MainWindow.h"
#include "ui/GeometryEditor.h"
#include "ui/MaterialEditor.h"
#include "ui/SensorEditor.h"
#include "ui/SourceEditor.h"
#include <QHeaderView>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QSplashScreen>
#include <QPixmap>
#include <QStyle>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QSplitter>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Core
    m_scene = std::make_shared<Scene>();
    m_engine = std::make_unique<MonteCarloEngine>(m_scene);
    // UI
    setupUI();
    connectSignals();

    // Settings
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // State
    updateSimulationControls();
    updateWindowTitle();

    Log::info("Interface principale initialisée");
}

MainWindow::~MainWindow() {
    if (m_engine && m_engine->isRunning()) {
        m_engine->stopSimulation();
    }
}

void MainWindow::setupUI() {
    setupCentralWidget();
    setupMenus();
    setupToolbars();
    setupStatusBar();
    setupDockWidgets();

    // Timers
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(100); // 10 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateSimulation);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(1000); // 1 Hz
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_statusTimer->start();
}

void MainWindow::setupCentralWidget() {
    // Splitter horizontal : [viewport] | [panneau simulation]
    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // Viewport (OpenGL si dispo, sinon QWidget fallback avec message)
#ifdef HAS_QOPENGLWIDGET
    m_view3D = new View3D();
    m_view3D->setScene(m_scene);
    m_view3D->setWireframeEnabled(false);
    m_view3D->setShowSensors(true);
    m_view3D->setShowSources(true);
    m_viewport = QWidget::createWindowContainer(m_view3D, this);
#else
    m_view3D = nullptr;
    m_viewport = new QWidget(this);
    auto *msg = new QLabel("OpenGL non disponible — affichage simplifié", m_viewport);
    msg->setAlignment(Qt::AlignCenter);
    auto *vbox = new QVBoxLayout(m_viewport);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(msg);
#endif
    m_viewport->setMinimumSize(600, 400);

    // Panneau simulation (droite)
    m_simulationPanel = new QWidget(this);
    m_simulationPanel->setMaximumWidth(340);
    m_simulationPanel->setMinimumWidth(260);
    setupSimulationPanel();

    mainSplitter->addWidget(m_viewport);
    mainSplitter->addWidget(m_simulationPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 0);

}

void MainWindow::setupSimulationPanel() {
    auto *layout = new QVBoxLayout(m_simulationPanel);

    // Contrôles de simulation
    auto *controlGroup = new QGroupBox("Contrôles de Simulation", this);
    auto *controlLayout = new QVBoxLayout(controlGroup);
    auto *buttonLayout = new QHBoxLayout();

    m_startButton = new QPushButton("Démarrer", this);
    m_pauseButton = new QPushButton("Pause", this);
    m_stopButton = new QPushButton("Arrêter", this);
    m_resetButton = new QPushButton("Reset", this);

    m_startButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_resetButton);

    m_progressBar = new QProgressBar(this);
    m_statusLabel = new QLabel("Prêt", this);

    controlLayout->addLayout(buttonLayout);
    controlLayout->addWidget(m_progressBar);
    controlLayout->addWidget(m_statusLabel);

    // Configuration
    auto *configGroup = new QGroupBox("Configuration", this);
    auto *configLayout = new QGridLayout(configGroup);

    // Particules
    configLayout->addWidget(new QLabel("Particules:"), 0, 0);
    m_particleCountSpin = new QSpinBox(this);
    m_particleCountSpin->setRange(1000, 10000000);
    m_particleCountSpin->setValue(100000);
    m_particleCountSpin->setSingleStep(10000);
    configLayout->addWidget(m_particleCountSpin, 0, 1);

    // Threads
    configLayout->addWidget(new QLabel("Threads:"), 1, 0);
    m_threadCountSpin = new QSpinBox(this);
    int hw = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    m_threadCountSpin->setRange(1, hw);
    m_threadCountSpin->setValue(hw);
    configLayout->addWidget(m_threadCountSpin, 1, 1);

    // Cutoff énergie
    configLayout->addWidget(new QLabel("Énergie min (keV):"), 2, 0);
    m_energyCutoffSpin = new QDoubleSpinBox(this);
    m_energyCutoffSpin->setRange(0.1, 1000.0);
    m_energyCutoffSpin->setValue(1.0);
    m_energyCutoffSpin->setDecimals(1);
    configLayout->addWidget(m_energyCutoffSpin, 2, 1);

    // Options
    m_backgroundSubtractionCheck = new QCheckBox("Soustraction du fond", this);
    m_backgroundSubtractionCheck->setChecked(true);
    m_varianceReductionCheck = new QCheckBox("Réduction de variance", this);
    m_varianceReductionCheck->setChecked(true);

    configLayout->addWidget(m_backgroundSubtractionCheck, 3, 0, 1, 2);
    configLayout->addWidget(m_varianceReductionCheck, 4, 0, 1, 2);

    // Résultats rapides
    m_resultsPanel = new QWidget(this);
    setupResultsPanel();

    layout->addWidget(controlGroup);
    layout->addWidget(configGroup);
    layout->addWidget(m_resultsPanel);
    layout->addStretch();
}

void MainWindow::setupResultsPanel() {
    auto *layout = new QVBoxLayout(m_resultsPanel);

    auto *resultsGroup = new QGroupBox("Résultats Rapides", this);
    auto *resultsLayout = new QVBoxLayout(resultsGroup);

    m_resultsTable = new QTableWidget(0, 2, this);
    m_resultsTable->setHorizontalHeaderLabels({"Paramètre", "Valeur"});
    m_resultsTable->setMaximumHeight(200);
    if (m_resultsTable->horizontalHeader())
        m_resultsTable->horizontalHeader()->setStretchLastSection(true);

    resultsLayout->addWidget(m_resultsTable);
    layout->addWidget(resultsGroup);
}

void MainWindow::setupMenus() {
    QMenuBar *mb = menuBar();

    // Fichier
    QMenu *fileMenu = mb->addMenu("&Fichier");
    auto *newAction = fileMenu->addAction("&Nouveau", this, &MainWindow::newProject);
    newAction->setShortcut(QKeySequence::New);
    newAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));

    auto *openAction = fileMenu->addAction("&Ouvrir...", this, &MainWindow::openProject);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));

    m_importVtkAction = fileMenu->addAction("Importer &VTK...", this, &MainWindow::importVtk);
    if (m_view3D)
        m_importVtkAction->setEnabled(true);
    else if (m_importVtkAction)
        m_importVtkAction->setEnabled(false);

    fileMenu->addSeparator();

    auto *saveAction = fileMenu->addAction("&Sauvegarder", this, &MainWindow::saveProject);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));

    auto *saveAsAction = fileMenu->addAction("Sauvegarder &sous...", this, &MainWindow::saveProjectAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    fileMenu->addSeparator();

    fileMenu->addAction("&Exporter résultats...", this, &MainWindow::exportResults);
    fileMenu->addSeparator();
    auto *exitAction = fileMenu->addAction("&Quitter", this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);

    // Simulation
    QMenu *simMenu = mb->addMenu("&Simulation");
    simMenu->addAction("&Démarrer", this, &MainWindow::startSimulation)->setShortcut(Qt::Key_F5);
    simMenu->addAction("&Pause", this, &MainWindow::pauseSimulation)->setShortcut(Qt::Key_F6);
    simMenu->addAction("&Arrêter", this, &MainWindow::stopSimulation)->setShortcut(Qt::Key_F7);
    simMenu->addAction("&Reset", this, &MainWindow::resetSimulation)->setShortcut(Qt::Key_F8);

    // Vue
    QMenu *viewMenu = mb->addMenu("&Vue");
    viewMenu->addAction("Reset &caméra", this, &MainWindow::resetCamera)->setShortcut(Qt::Key_Home);
    viewMenu->addSeparator();
    m_wireframeAction = viewMenu->addAction("Mode &filaire", this, &MainWindow::toggleWireframe);
    m_wireframeAction->setCheckable(true);
    m_wireframeAction->setChecked(false);
    m_showSensorsAction = viewMenu->addAction("Afficher &capteurs", this, &MainWindow::toggleSensors);
    m_showSensorsAction->setCheckable(true);
    m_showSensorsAction->setChecked(true);
    m_showSourcesAction = viewMenu->addAction("Afficher &sources", this, &MainWindow::toggleSources);
    m_showSourcesAction->setCheckable(true);
    m_showSourcesAction->setChecked(true);

    // Aide
    QMenu *helpMenu = mb->addMenu("&Aide");
    helpMenu->addAction("À &propos...", this, &MainWindow::showAbout);
}

void MainWindow::setupToolbars() {
    QToolBar *tb = addToolBar("Principal");
    tb->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Nouveau", this, &MainWindow::newProject);
    tb->addAction(style()->standardIcon(QStyle::SP_DirOpenIcon), "Ouvrir", this, &MainWindow::openProject);
    tb->addAction(style()->standardIcon(QStyle::SP_DriveFDIcon), "Sauvegarder", this, &MainWindow::saveProject);
    tb->addSeparator();
    tb->addAction(style()->standardIcon(QStyle::SP_MediaPlay), "Démarrer", this, &MainWindow::startSimulation);
    tb->addAction(style()->standardIcon(QStyle::SP_MediaPause), "Pause", this, &MainWindow::pauseSimulation);
    tb->addAction(style()->standardIcon(QStyle::SP_MediaStop), "Arrêter", this, &MainWindow::stopSimulation);
    tb->addSeparator();
    tb->addAction("Reset caméra", this, &MainWindow::resetCamera);
}

void MainWindow::importVtk() {
    if (!m_view3D) {
        QMessageBox::warning(this, tr("Import VTK"), tr("Le rendu 3D n'est pas disponible sur cette plateforme."));
        return;
    }

    const QString filter = tr("Fichiers VTK (*.vtk)");
    QString filePath = QFileDialog::getOpenFileName(this, tr("Importer un fichier VTK"), QString(), filter);
    if (filePath.isEmpty())
        return;

    std::string error;
    if (!m_view3D->importVtkMesh(filePath, &error)) {
        QString message = error.empty() ? tr("Impossible de charger le fichier VTK sélectionné.") : QString::fromStdString(error);
        QMessageBox::critical(this, tr("Import VTK"), message);
        Log::error("Import VTK échoué: " + filePath.toStdString() + (error.empty() ? std::string() : (" (" + error + ")")));
        return;
    }

    QFileInfo info(filePath);
    const QString status = tr("Maillage VTK chargé: %1").arg(info.fileName());
    statusBar()->showMessage(status, 5000);
    if (m_viewport)
        m_viewport->update();
}

void MainWindow::setupStatusBar() {
    QStatusBar *status = statusBar();
    m_statusLabel = new QLabel("Prêt", this);
    status->addWidget(m_statusLabel);
    status->addPermanentWidget(new QLabel("Particules: 0", this));
    status->addPermanentWidget(new QLabel("Temps: 0s", this));
}

void MainWindow::setupDockWidgets() {
    // Géométrie
    m_geometryDock = new QDockWidget("Géométrie", this);
    m_geometryEditor = new GeometryEditor(m_scene, this);
    m_geometryDock->setWidget(m_geometryEditor);
    addDockWidget(Qt::LeftDockWidgetArea, m_geometryDock);

    // Matériaux
    m_materialDock = new QDockWidget("Matériaux", this);
    m_materialEditor = new MaterialEditor(m_scene, this);
    m_materialDock->setWidget(m_materialEditor);
    addDockWidget(Qt::LeftDockWidgetArea, m_materialDock);

    // Capteurs
    m_sensorDock = new QDockWidget("Capteurs", this);
    m_sensorEditor = new SensorEditor(m_scene, this);
    m_sensorDock->setWidget(m_sensorEditor);
    addDockWidget(Qt::RightDockWidgetArea, m_sensorDock);

    // Sources
    m_sourceDock = new QDockWidget("Sources", this);
    m_sourceEditor = new SourceEditor(m_scene, this);
    m_sourceDock->setWidget(m_sourceEditor);
    addDockWidget(Qt::RightDockWidgetArea, m_sourceDock);

    // Résultats
    m_resultsDock = new QDockWidget("Résultats", this);
    m_sensorTree = new QTreeWidget(this);
    m_sensorTree->setHeaderLabels({"Capteur", "Valeur", "Unité"});
    m_resultsDock->setWidget(m_sensorTree);
    addDockWidget(Qt::BottomDockWidgetArea, m_resultsDock);

    // Journal
    m_logDock = new QDockWidget("Journal", this);
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(150);
    m_logDock->setWidget(m_logText);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);

    // Organisation
    tabifyDockWidget(m_geometryDock, m_materialDock);
    tabifyDockWidget(m_sensorDock, m_sourceDock);
    tabifyDockWidget(m_resultsDock, m_logDock);

    // Sélection par défaut
    m_geometryDock->raise();
    m_sensorDock->raise();
    m_resultsDock->raise();
}

void MainWindow::connectSignals() {
    // Boutons
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainWindow::pauseSimulation);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopSimulation);
    connect(m_resetButton, &QPushButton::clicked, this, &MainWindow::resetSimulation);

    // Config
    connect(m_particleCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        auto cfg = m_engine->getConfig();
        cfg.maxParticles = value;
        m_engine->setConfig(cfg);
        setProjectModified();
    });

    connect(m_threadCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        auto cfg = m_engine->getConfig();
        cfg.numThreads = value;
        m_engine->setConfig(cfg);
        setProjectModified();
    });
}

// Simulation
void MainWindow::startSimulation() {
    if (!m_engine)
        return;

    try {
        auto cfg = m_engine->getConfig();
        cfg.maxParticles = m_particleCountSpin->value();
        cfg.numThreads = m_threadCountSpin->value();
        cfg.energyCutoff = m_energyCutoffSpin->value();
        cfg.enableBackgroundSubtraction = m_backgroundSubtractionCheck->isChecked();
        cfg.enableVarianceReduction = m_varianceReductionCheck->isChecked();
        m_engine->setConfig(cfg);

        m_engine->startSimulation();
        m_updateTimer->start();
        updateSimulationControls();
        Log::info("Simulation démarrée");
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Erreur", QString("Impossible de démarrer la simulation:\n%1").arg(e.what()));
    }
}

void MainWindow::pauseSimulation() {
    if (m_engine) {
        m_engine->pauseSimulation();
        updateSimulationControls();
    }
}

void MainWindow::stopSimulation() {
    if (m_engine) {
        m_engine->stopSimulation();
        m_updateTimer->stop();
        updateSimulationControls();
        updateResults();
    }
}

void MainWindow::resetSimulation() {
    stopSimulation();
    m_engine->resetStats();
    m_progressBar->setValue(0);
    updateResults();
    Log::info("Simulation réinitialisée");
}

// Fichier
void MainWindow::newProject() {
    if (!saveChanges())
        return;

    m_scene->clear();
    m_currentProjectFile.clear();
    setProjectModified(false);
    resetSimulation();
    Log::info("Nouveau projet créé");
}

void MainWindow::openProject() {
    if (!saveChanges())
        return;

    QString filename = QFileDialog::getOpenFileName(this, "Ouvrir un projet", "", "Projets (*.radsim)");
    if (!filename.isEmpty()) {
        try {
            m_scene->loadFromFile(filename.toStdString());
            m_currentProjectFile = filename;
            setProjectModified(false);
            Log::info("Projet ouvert: " + filename.toStdString());
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "Erreur", QString("Impossible d'ouvrir le projet:\n%1").arg(e.what()));
        }
    }
}

void MainWindow::saveProject() {
    if (m_currentProjectFile.isEmpty()) {
        saveProjectAs();
    } else {
        try {
            m_scene->saveToFile(m_currentProjectFile.toStdString());
            setProjectModified(false);
            Log::info("Projet sauvegardé");
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "Erreur", QString("Impossible de sauvegarder:\n%1").arg(e.what()));
        }
    }
}

void MainWindow::saveProjectAs() {
    QString filename = QFileDialog::getSaveFileName(this, "Sauvegarder le projet", "", "Projets (*.radsim)");
    if (!filename.isEmpty()) {
        m_currentProjectFile = filename;
        saveProject();
    }
}

void MainWindow::exportResults() {
    QString filename = QFileDialog::getSaveFileName(this, "Exporter les résultats", "", "CSV (*.csv);;JSON (*.json)");
    if (!filename.isEmpty()) {
        // TODO: export réel
        Log::info("Résultats exportés vers: " + filename.toStdString());
    }
}

// Vue
void MainWindow::resetCamera() {
#ifdef HAS_QOPENGLWIDGET
    if (m_view3D) {
        m_view3D->resetCamera();
        return;
    }
#endif
}

void MainWindow::toggleWireframe() {
#ifdef HAS_QOPENGLWIDGET
    bool enabled = m_wireframeAction && m_wireframeAction->isChecked();
    if (m_view3D)
        m_view3D->setWireframeEnabled(enabled);
    else if (m_wireframeAction)
        m_wireframeAction->setChecked(false);
#endif
}

void MainWindow::toggleSensors() {
#ifdef HAS_QOPENGLWIDGET
    bool enabled = m_showSensorsAction ? m_showSensorsAction->isChecked() : true;
    if (m_view3D)
        m_view3D->setShowSensors(enabled);
#endif
}

void MainWindow::toggleSources() {
#ifdef HAS_QOPENGLWIDGET
    bool enabled = m_showSourcesAction ? m_showSourcesAction->isChecked() : true;
    if (m_view3D)
        m_view3D->setShowSources(enabled);
#endif
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "À propos",
                       "Simulateur d'Atténuation de Radiation v1.0\n\n"
                       "Simulation Monte Carlo d'atténuation de radiation\n"
                       "dans des enceintes blindées.\n\n"
                       "Développé avec Qt\n"
                       "© 2025 Physics Simulation Lab");
}

// Updates
void MainWindow::updateSimulation() {
    if (!m_engine)
        return;

    float progress = m_engine->getProgress();
    m_progressBar->setValue(static_cast<int>(progress * 100));

    const auto &stats = m_engine->getStats();
    m_statusLabel->setText(QString("Particules: %1").arg(stats.particlesTransported.load()));

    if (!m_engine->isRunning() && progress >= 1.0f) {
        m_updateTimer->stop();
        updateSimulationControls();
        Log::info("Simulation terminée");
    }
}

void MainWindow::updateStatusBar() {
    if (!m_engine)
        return;

    const auto &stats = m_engine->getStats();
    double elapsed = stats.getElapsedTime();
    double rate = stats.getParticleRate();

    statusBar()->showMessage(QString("Temps: %1s | Taux: %2 part/s").arg(elapsed, 0, 'f', 1).arg(rate, 0, 'f', 0));
}

void MainWindow::updateResults() {
    if (!m_scene)
        return;

    m_resultsTable->setRowCount(0);
    auto sensors = m_scene->getAllSensors();
    for (auto &sensor : sensors) {
        int row = m_resultsTable->rowCount();
        m_resultsTable->insertRow(row);
        m_resultsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(sensor->getName())));
        m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(sensor->getStats().totalCounts.load())));
    }
}

void MainWindow::updateSimulationControls() {
    if (!m_engine)
        return;

    bool running = m_engine->isRunning();
    bool paused = m_engine->getState() == SimulationState::PAUSED;

    m_startButton->setEnabled(!running || paused);
    m_pauseButton->setEnabled(running && !paused);
    m_stopButton->setEnabled(running);

    // Lock config quand ça tourne
    m_particleCountSpin->setEnabled(!running);
    m_threadCountSpin->setEnabled(!running);
    m_energyCutoffSpin->setEnabled(!running);
}

void MainWindow::setProjectModified(bool modified) {
    m_projectModified = modified;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle() {
    QString title = "Simulateur d'Atténuation de Radiation";

    if (!m_currentProjectFile.isEmpty()) {
        QFileInfo info(m_currentProjectFile);
        title += " - " + info.baseName();
    } else {
        title += " - Nouveau projet";
    }

    if (m_projectModified) {
        title += " *";
    }
    setWindowTitle(title);
}

bool MainWindow::saveChanges() {
    if (!m_projectModified)
        return true;

    auto ret = QMessageBox::question(this, "Sauvegarder les modifications",
                                     "Le projet a été modifié. Voulez-vous sauvegarder les modifications?",
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        saveProject();
        return !m_projectModified;
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (saveChanges()) {
        QSettings settings;
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());

        if (m_engine && m_engine->isRunning()) {
            m_engine->stopSimulation();
        }
        event->accept();
    } else {
        event->ignore();
    }
}
