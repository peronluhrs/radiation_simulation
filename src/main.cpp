#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <iostream>

#include "ui/MainWindow.h"
#include "core/Material.h"
#include "core/Scene.h"
#include "common.h"

// Initialisation des générateurs aléatoires thread-local

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Configuration de l'application
    app.setApplicationName("Radiation Attenuation Simulator");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Physics Simulation Lab");

    // Style moderne
    app.setStyle(QStyleFactory::create("Fusion"));

    // Palette sombre
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    try
    {
        // Initialisation des bibliothèques par défaut
        Log::info("Initialisation du simulateur d'atténuation de radiation...");

        MaterialLibrary::getInstance().loadDefaults();
        Log::info("Bibliothèque de matériaux chargée");

        // Création de la fenêtre principale
        MainWindow window;
        window.show();

        Log::info("Interface utilisateur prête");

        return app.exec();
    }
    catch (const std::exception &e)
    {
        Log::error("Erreur fatale: " + std::string(e.what()));
        return -1;
    }
}

namespace Log
{
    void info(const std::string &message)
    {
        std::cout << "[INFO] " << message << std::endl;
    }

    void warning(const std::string &message)
    {
        std::cout << "[WARNING] " << message << std::endl;
    }

    void error(const std::string &message)
    {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}