#pragma once

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
#include <QWidget>      // fallback viewport type

// OpenGL widget optionnel
#if __has_include(<QOpenGLWidget>)
  #include <QOpenGLWidget>
  #define HAS_QOPENGLWIDGET 1
#endif

#include "common.h"
#include "core/Scene.h"
#include "simulation/MonteCarloEngine.h"
#include "visualization/Renderer.h"

// Forward declarations des éditeurs
class GeometryEditor;
class MaterialEditor;
class SensorEditor;
class SourceEditor;

class MainWindow : public QMainWindow
{
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
    // Construction UI
    void setupUI();
    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void setupDockWidgets();
    void setupCentralWidget();
    void setupSimulationPanel();
    void setupResultsPanel();

    void connectSignals();
    void updateSimulationControls();

    // Core
    std::shared_ptr<Scene> m_scene;
    std::unique_ptr<MonteCarloEngine> m_engine;
    std::unique_ptr<Renderer> m_renderer;

    // Viewport : OpenGL si dispo, sinon QWidget
#ifdef HAS_QOPENGLWIDGET
    QOpenGLWidget *m_viewport {nullptr};
#else
    QWidget       *m_viewport {nullptr};
#endif

    // Editors
    GeometryEditor *m_geometryEditor {nullptr};
    MaterialEditor *m_materialEditor {nullptr};
    SensorEditor   *m_sensorEditor   {nullptr};
    SourceEditor   *m_sourceEditor   {nullptr};

    // Dock widgets
    QDockWidget *m_geometryDock {nullptr};
    QDockWidget *m_materialDock {nullptr};
    QDockWidget *m_sensorDock   {nullptr};
    QDockWidget *m_sourceDock   {nullptr};
    QDockWidget *m_resultsDock  {nullptr};
    QDockWidget *m_logDock      {nullptr};

    // Control panels
    QWidget   *m_simulationPanel {nullptr};
    QWidget   *m_resultsPanel    {nullptr};
    QTextEdit *m_logText         {nullptr};

    // Simulation controls
    QPushButton   *m_startButton  {nullptr};
    QPushButton   *m_pauseButton  {nullptr};
    QPushButton   *m_stopButton   {nullptr};
    QPushButton   *m_resetButton  {nullptr};
    QProgressBar  *m_progressBar  {nullptr};
    QLabel        *m_statusLabel  {nullptr};

    // Configuration
    QSpinBox        *m_particleCountSpin        {nullptr};
    QSpinBox        *m_threadCountSpin          {nullptr};
    QDoubleSpinBox  *m_energyCutoffSpin         {nullptr};
    QCheckBox       *m_backgroundSubtractionCheck {nullptr};
    QCheckBox       *m_varianceReductionCheck     {nullptr};

    // Results display
    QTableWidget *m_resultsTable {nullptr};
    QTreeWidget  *m_sensorTree   {nullptr};

    // Timers
    QTimer *m_updateTimer {nullptr};
    QTimer *m_statusTimer {nullptr};

    // State
    QString m_currentProjectFile;
    bool    m_projectModified = false;

    void setProjectModified(bool modified = true);
    void updateWindowTitle();
    bool saveChanges();
};
