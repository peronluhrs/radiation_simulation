#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QTabWidget>
#include <QTimer>
#include <QAction>

#include "common.h"
#include "core/Scene.h"
#include "simulation/MonteCarloEngine.h"

// On n’inclut plus QOpenGLWidget ici : le viewport est un QWidget container
// pour un QOpenGLWindow (View3D). Ça évite les conversions invalides.
// #include <QOpenGLWidget>  // <- NE PAS inclure

// forward declares des éditeurs (headers dans include/ui/*.h)
class GeometryEditor;
class MaterialEditor;
class SensorEditor;
class SourceEditor;
class View3D;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

  protected:
    void closeEvent(QCloseEvent *event) override;

  private slots:
    // Menu actions
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportResults();
    void importVtk();

    // Simulation control
    void startSimulation();
    void pauseSimulation();
    void stopSimulation();
    void resetSimulation();

    // View control
    void resetCamera();
    void toggleWireframe();
    void toggleSensors();
    void toggleSources();
    void showAbout();

    // Updates
    void updateSimulation();
    void updateStatusBar();
    void updateResults();

  private:
    void setupSimulationPanel();
    void setupResultsPanel();
    void setupUI();
    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void setupDockWidgets();
    void setupCentralWidget();

    void connectSignals();
    void updateSimulationControls();

    // Core components
    std::shared_ptr<Scene> m_scene;
    std::unique_ptr<MonteCarloEngine> m_engine;

    // UI / rendu : container QWidget pour le QOpenGLWindow (View3D)
    QWidget *m_viewport = nullptr;
    View3D *m_view3D = nullptr;

    // Editors
    GeometryEditor *m_geometryEditor = nullptr;
    MaterialEditor *m_materialEditor = nullptr;
    SensorEditor *m_sensorEditor = nullptr;
    SourceEditor *m_sourceEditor = nullptr;

    // Dock widgets
    QDockWidget *m_geometryDock = nullptr;
    QDockWidget *m_materialDock = nullptr;
    QDockWidget *m_sensorDock = nullptr;
    QDockWidget *m_sourceDock = nullptr;
    QDockWidget *m_resultsDock = nullptr;
    QDockWidget *m_logDock = nullptr;

    // Control panels
    QWidget *m_simulationPanel = nullptr;
    QWidget *m_resultsPanel = nullptr;
    QTextEdit *m_logText = nullptr;

    // Simulation controls
    QPushButton *m_startButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Configuration
    QSpinBox *m_particleCountSpin = nullptr;
    QSpinBox *m_threadCountSpin = nullptr;
    QDoubleSpinBox *m_energyCutoffSpin = nullptr;
    QCheckBox *m_backgroundSubtractionCheck = nullptr;
    QCheckBox *m_varianceReductionCheck = nullptr;

    // Results display
    QTableWidget *m_resultsTable = nullptr;
    QTreeWidget *m_sensorTree = nullptr;

    // Timers
    QTimer *m_updateTimer = nullptr;
    QAction *m_wireframeAction = nullptr;
    QAction *m_showSensorsAction = nullptr;
    QAction *m_showSourcesAction = nullptr;

    QTimer *m_statusTimer = nullptr;

    // State
    QString m_currentProjectFile;
    bool m_projectModified = false;

    void setProjectModified(bool modified = true);
    void updateWindowTitle();
    bool saveChanges();
};
