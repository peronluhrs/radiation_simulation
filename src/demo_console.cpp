#include "common.h"
#include "core/Scene.h"
#include "core/Material.h"
#include "core/Sensor.h"
#include "core/Source.h"
#include "geometry/Box.h"
#include "geometry/Sphere.h"
#include "simulation/MonteCarloEngine.h"

#include <iostream>
#include <iomanip>
#include <chrono>

// Version console pour démonstration sans Qt
class ConsoleDemo {
public:
    static void runDemo() {
        std::cout << "=== SIMULATEUR D'ATTÉNUATION DE RADIATION ===" << std::endl;
        std::cout << "Version Console de Démonstration" << std::endl;
        std::cout << "=============================================" << std::endl << std::endl;
        
        try {
            // Initialisation
            initializeMaterials();
            
            // Création de la scène de test
            auto scene = createTestScene();
            
            // Configuration de la simulation
            SimulationConfig config = getTestConfig();
            
            // Exécution de la simulation
            runSimulation(scene, config);
            
        } catch (const std::exception& e) {
            std::cerr << "ERREUR: " << e.what() << std::endl;
        }
    }
    
private:
    static void initializeMaterials() {
        std::cout << "Initialisation de la bibliothèque de matériaux..." << std::endl;
        MaterialLibrary::getInstance().loadDefaults();
        
        auto materials = MaterialLibrary::getInstance().getMaterialNames();
        std::cout << "Matériaux disponibles: ";
        for (size_t i = 0; i < materials.size(); ++i) {
            std::cout << materials[i];
            if (i < materials.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl << std::endl;
    }
    
    static std::shared_ptr<Scene> createTestScene() {
        std::cout << "Création de la scène de test..." << std::endl;
        
        auto scene = std::make_shared<Scene>();
        auto& materials = MaterialLibrary::getInstance();
        
        // === GÉOMÉTRIE SIMPLE ===
        
        // Mur de plomb (5cm d'épaisseur)
        auto leadWall = std::make_shared<Box>("Mur_Plomb", glm::vec3(2.0f, 2.0f, 0.05f));
        leadWall->setMaterial(materials.getMaterial("Plomb"));
        leadWall->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        scene->addObject(leadWall);
        
        // Mur de béton (30cm d'épaisseur)
        auto concreteWall = std::make_shared<Box>("Mur_Beton", glm::vec3(2.0f, 2.0f, 0.3f));
        concreteWall->setMaterial(materials.getMaterial("Béton"));
        concreteWall->setPosition(glm::vec3(0.0f, 0.0f, 0.5f));
        scene->addObject(concreteWall);
        
        // === SOURCE GAMMA ===
        
        // Source Cs-137 (662 keV)
        auto gammaSource = std::make_shared<IsotropicSource>("Cs-137", RadiationType::GAMMA);
        gammaSource->setPosition(glm::vec3(0.0f, 0.0f, -1.0f)); // 1m avant le mur
        gammaSource->setIntensity(1e6); // 1 MBq
        
        EnergySpectrum spectrum;
        spectrum.type = EnergySpectrum::MONOENERGETIC;
        spectrum.energy = 662.0f; // keV
        gammaSource->setSpectrum(spectrum);
        
        scene->addSource(gammaSource);
        
        // === CAPTEURS ===
        
        // Capteur avant le mur (référence)
        auto sensor1 = std::make_shared<Sensor>("Avant_Blindage", SensorType::POINT, 
                                               glm::vec3(0.0f, 0.0f, -0.5f));
        scene->addSensor(sensor1);
        
        // Capteur après le mur de plomb
        auto sensor2 = std::make_shared<Sensor>("Apres_Plomb", SensorType::POINT, 
                                               glm::vec3(0.0f, 0.0f, 0.1f));
        scene->addSensor(sensor2);
        
        // Capteur après le mur de béton
        auto sensor3 = std::make_shared<Sensor>("Apres_Beton", SensorType::POINT, 
                                               glm::vec3(0.0f, 0.0f, 1.0f));
        scene->addSensor(sensor3);
        
        // Construction de la structure d'accélération
        scene->buildAccelerationStructure();
        
        std::cout << "Scène créée:" << std::endl;
        std::cout << "  - " << scene->getObjectCount() << " objets" << std::endl;
        std::cout << "  - " << scene->getSensorCount() << " capteurs" << std::endl;
        std::cout << "  - " << scene->getSourceCount() << " sources" << std::endl;
        std::cout << std::endl;
        
        return scene;
    }
    
    static SimulationConfig getTestConfig() {
        SimulationConfig config;
        
        // Configuration simplifiée pour démonstration rapide
        config.maxParticles = 50000;        // 50k particules pour être rapide
        config.maxBounces = 20;
        config.energyCutoff = 10.0f;        // 10 keV
        config.timeCutoff = 1e6f;           // 1 ms
        
        config.enableBackgroundSubtraction = false; // Simplification
        config.enableVarianceReduction = true;
        
        config.useRussianRoulette = true;
        config.russianRouletteThreshold = 0.1f;
        
        config.useSplitting = false;
        config.numThreads = std::min(4u, std::thread::hardware_concurrency());
        
        return config;
    }
    
    static void runSimulation(std::shared_ptr<Scene> scene, const SimulationConfig& config) {
        std::cout << "Configuration de la simulation:" << std::endl;
        std::cout << "  - " << config.maxParticles << " particules maximum" << std::endl;
        std::cout << "  - " << config.numThreads << " threads de calcul" << std::endl;
        std::cout << "  - Seuil d'énergie: " << config.energyCutoff << " keV" << std::endl;
        std::cout << std::endl;
        
        // Création du moteur Monte Carlo
        MonteCarloEngine engine(scene);
        engine.setConfig(config);
        
        std::cout << "Démarrage de la simulation..." << std::endl;
        auto startTime = std::chrono::steady_clock::now();
        
        engine.startSimulation();
        
        // Affichage du progrès
        std::cout << "Progrès: [";
        const int barWidth = 50;
        
        while (engine.isRunning()) {
            float progress = engine.getProgress();
            const auto& stats = engine.getStats();
            
            // Barre de progression
            int pos = static_cast<int>(barWidth * progress);
            std::cout << "\rProgrès: [";
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << std::fixed << std::setprecision(1) 
                      << (progress * 100.0f) << "% (" 
                      << stats.particlesTransported.load() << " particules)" << std::flush;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << std::endl << std::endl;
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "Simulation terminée en " << duration.count() << " ms" << std::endl;
        std::cout << std::endl;
        
        // Affichage des résultats
        displayResults(scene, engine.getStats());
        
        // Calculs analytiques pour comparaison
        displayAnalyticalComparison(scene);
    }
    
    static void displayResults(std::shared_ptr<Scene> scene, const SimulationStats& stats) {
        std::cout << "=== RÉSULTATS DE SIMULATION ===" << std::endl;
        
        // Statistiques générales
        std::cout << "Statistiques générales:" << std::endl;
        std::cout << "  Particules émises:      " << stats.particlesEmitted.load() << std::endl;
        std::cout << "  Particules transportées: " << stats.particlesTransported.load() << std::endl;
        std::cout << "  Particules absorbées:   " << stats.particlesAbsorbed.load() << std::endl;
        std::cout << "  Particules détectées:   " << stats.particlesDetected.load() << std::endl;
        std::cout << "  Particules échappées:   " << stats.particlesEscaped.load() << std::endl;
        std::cout << "  Intersections de rayons: " << stats.rayIntersections.load() << std::endl;
        std::cout << "  Taux de simulation:     " << std::fixed << std::setprecision(0) 
                  << stats.getParticleRate() << " particules/s" << std::endl;
        std::cout << std::endl;
        
        // Résultats par capteur
        std::cout << "Détections par capteur:" << std::endl;
        std::cout << std::setw(20) << "Capteur" 
                  << std::setw(12) << "Total" 
                  << std::setw(12) << "Gamma" 
                  << std::setw(15) << "Énergie (keV)" 
                  << std::setw(15) << "Dose (μSv/h)" << std::endl;
        std::cout << std::string(74, '-') << std::endl;
        
        auto sensors = scene->getAllSensors();
        std::shared_ptr<Sensor> referenceSensor = nullptr;
        
        for (auto& sensor : sensors) {
            const auto& sensorStats = sensor->getStats();
            
            if (sensor->getName() == "Avant_Blindage") {
                referenceSensor = sensor;
            }
            
            std::cout << std::setw(20) << sensor->getName()
                      << std::setw(12) << sensorStats.totalCounts.load()
                      << std::setw(12) << sensorStats.gammaCounts.load()
                      << std::setw(15) << std::fixed << std::setprecision(1) 
                      << sensorStats.totalEnergy.load()
                      << std::setw(15) << std::setprecision(3) 
                      << sensor->getDoseRate() << std::endl;
        }
        
        std::cout << std::endl;
        
        // Facteurs d'atténuation
        if (referenceSensor) {
            std::cout << "Facteurs d'atténuation (par rapport à 'Avant_Blindage'):" << std::endl;
            double referenceCounts = referenceSensor->getStats().totalCounts.load();
            
            for (auto& sensor : sensors) {
                if (sensor == referenceSensor) continue;
                
                double sensorCounts = sensor->getStats().totalCounts.load();
                double attenuationFactor = referenceCounts > 0 ? sensorCounts / referenceCounts : 0.0;
                
                std::cout << "  " << std::setw(20) << sensor->getName() 
                          << ": " << std::fixed << std::setprecision(6) 
                          << attenuationFactor 
                          << " (atténuation: " << std::setprecision(1) 
                          << (1.0 - attenuationFactor) * 100.0 << "%)" << std::endl;
            }
        }
        
        std::cout << std::endl;
    }
    
    static void displayAnalyticalComparison(std::shared_ptr<Scene> scene) {
        std::cout << "=== COMPARAISON ANALYTIQUE ===" << std::endl;
        
        auto& materials = MaterialLibrary::getInstance();
        auto lead = materials.getMaterial("Plomb");
        auto concrete = materials.getMaterial("Béton");
        
        if (lead && concrete) {
            float energy = 662.0f; // keV Cs-137

            // Coefficients d'atténuation
            float muLead_m = lead->getLinearAttenuationPerMeter(RadiationType::GAMMA, energy);
            float muConcrete_m = concrete->getLinearAttenuationPerMeter(RadiationType::GAMMA, energy);
            float muLead_cm = muLead_m / 100.0f;
            float muConcrete_cm = muConcrete_m / 100.0f;

            float leadThicknessM = 0.05f;
            float concreteThicknessM = 0.3f;

            if (auto leadObject = std::dynamic_pointer_cast<Box>(scene->getObject("Mur_Plomb"))) {
                leadThicknessM = leadObject->getDepth();
            }
            if (auto concreteObject = std::dynamic_pointer_cast<Box>(scene->getObject("Mur_Beton"))) {
                concreteThicknessM = concreteObject->getDepth();
            }

            float leadThicknessCm = leadThicknessM * 100.0f;
            float concreteThicknessCm = concreteThicknessM * 100.0f;

            std::cout << "Coefficients d'atténuation à " << energy << " keV:" << std::endl;
            std::cout << "  Plomb:  μ = " << std::fixed << std::setprecision(3)
                      << muLead_cm << " cm⁻¹" << std::endl;
            std::cout << "  Béton:  μ = " << std::fixed << std::setprecision(3)
                      << muConcrete_cm << " cm⁻¹" << std::endl;
            std::cout << std::endl;

            // Atténuation théorique
            float leadAttenuation = std::exp(-muLead_cm * leadThicknessCm);
            float concreteAttenuation = std::exp(-muConcrete_cm * concreteThicknessCm);
            float totalAttenuation = leadAttenuation * concreteAttenuation;

            std::cout << "Atténuation théorique (loi exponentielle):" << std::endl;
            std::cout << "  Plomb (" << leadThicknessCm << " cm):      "
                      << std::fixed << std::setprecision(6) << leadAttenuation
                      << " (" << std::setprecision(1) << (1.0f - leadAttenuation) * 100.0f << "% d'atténuation)" << std::endl;
            std::cout << "  Béton (" << concreteThicknessCm << " cm):     "
                      << std::fixed << std::setprecision(6) << concreteAttenuation
                      << " (" << std::setprecision(1) << (1.0f - concreteAttenuation) * 100.0f << "% d'atténuation)" << std::endl;
            std::cout << "  Total (Pb + Béton):    "
                      << std::fixed << std::setprecision(6) << totalAttenuation 
                      << " (" << std::setprecision(1) << (1.0f - totalAttenuation) * 100.0f << "% d'atténuation)" << std::endl;
            
            std::cout << std::endl;
            std::cout << "Note: Les résultats Monte Carlo peuvent différer légèrement" << std::endl;
            std::cout << "      en raison des effets statistiques et de diffusion." << std::endl;
        }
        
        std::cout << std::endl;
    }
};

// Point d'entrée pour la démonstration console
int main(int argc, char* argv[]) {
    // Initialisation des générateurs aléatoires
    RandomGenerator::generator.seed(std::random_device{}());
    
    // Arguments de ligne de commande simples
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Simulateur d'Atténuation de Radiation - Version Console" << std::endl;
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << std::endl;
            std::cout << "OPTIONS:" << std::endl;
            std::cout << "  --help, -h    Afficher cette aide" << std::endl;
            std::cout << "  --version     Afficher la version" << std::endl;
            std::cout << std::endl;
            return 0;
        } else if (arg == "--version") {
            std::cout << "Version 1.0.0 - Démonstration Console" << std::endl;
            return 0;
        }
    }
    
    try {
        ConsoleDemo::runDemo();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Erreur fatale: " << e.what() << std::endl;
        return 1;
    }
}