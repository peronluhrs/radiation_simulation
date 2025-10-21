#pragma once

#include "common.h"
#include "simulation/Particle.h"
#include "core/Scene.h"

// Configuration de simulation
struct SimulationConfig {
    uint64_t maxParticles = 1000000;
    uint32_t maxBounces = 100;
    float energyCutoff = 1.0f; // keV
    float timeCutoff = 1e6f;   // ns
    bool enableBackgroundSubtraction = true;
    bool enableVarianceReduction = true;
    uint32_t numThreads = std::thread::hardware_concurrency();
    
    // Optimisations
    bool useRussianRoulette = true;
    float russianRouletteThreshold = 0.1f;
    bool useSplitting = false;
    uint32_t splittingFactor = 2;
};

// Statistiques de simulation
struct SimulationStats {
    std::atomic<uint64_t> particlesEmitted{0};
    std::atomic<uint64_t> particlesTransported{0};
    std::atomic<uint64_t> particlesAbsorbed{0};
    std::atomic<uint64_t> particlesDetected{0};
    std::atomic<uint64_t> particlesEscaped{0};
    std::atomic<uint64_t> totalCollisions{0};
    std::atomic<uint64_t> rayIntersections{0};
    
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    
    void clear() {
        particlesEmitted = 0;
        particlesTransported = 0;
        particlesAbsorbed = 0;
        particlesDetected = 0;
        particlesEscaped = 0;
        totalCollisions = 0;
        rayIntersections = 0;
    }
    
    double getElapsedTime() const {
        auto end = (endTime > startTime) ? endTime : std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - startTime).count();
    }
    
    double getParticleRate() const {
        double elapsed = getElapsedTime();
        return elapsed > 0.0 ? particlesTransported.load() / elapsed : 0.0;
    }
};

// État de simulation
enum class SimulationState {
    IDLE,
    RUNNING,
    PAUSED,
    COMPLETED,
    ERROR
};

class MonteCarloEngine {
public:
    MonteCarloEngine(std::shared_ptr<Scene> scene);
    ~MonteCarloEngine();

    // Configuration
    const SimulationConfig& getConfig() const { return m_config; }
    void setConfig(const SimulationConfig& config) { m_config = config; }
    
    // Contrôle de simulation
    void startSimulation();
    void pauseSimulation();
    void resumeSimulation();
    void stopSimulation();
    bool isRunning() const { return m_state == SimulationState::RUNNING; }
    SimulationState getState() const { return m_state; }
    
    // Simulation progressive (pour interface temps réel)
    void runBatch(uint32_t numParticles = 1000);
    float getProgress() const;
    
    // Statistiques
    const SimulationStats& getStats() const { return m_stats; }
    void resetStats() { m_stats.clear(); }
    
    // Transport de particule unique (pour debugging)
    void transportParticle(Particle& particle);
    
    // Réduction de variance
    void enableRussianRoulette(bool enable, float threshold = 0.1f);
    void enableSplitting(bool enable, uint32_t factor = 2);
    void enableImportanceSampling(bool enable);

private:
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<Material> m_worldMaterial;
    SimulationConfig m_config;
    SimulationStats m_stats;
    SimulationState m_state = SimulationState::IDLE;
    
    // Threading
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_shouldStop{false};
    std::atomic<bool> m_shouldPause{false};
    std::mutex m_stateMutex;
    std::condition_variable m_pauseCondition;
    
    // Générateurs aléatoires par thread
    thread_local static std::mt19937 s_rng;
    
    // Worker functions
    void workerThread(uint32_t threadId);
    void emitAndTransportBatch(uint32_t batchSize, uint32_t threadId);
    
    // Transport de particule
    void transportParticleInternal(Particle& particle);
    bool stepParticle(Particle& particle);
    
    // Interactions physiques
    InteractionType sampleInteraction(const Particle& particle, std::shared_ptr<Material> material);
    void processInteraction(Particle& particle, InteractionType interaction, std::shared_ptr<Material> material);
    
    // Scattering
    glm::vec3 sampleComptonScattering(const Particle& particle, std::shared_ptr<Material> material);
    glm::vec3 sampleNeutronScattering(const Particle& particle, std::shared_ptr<Material> material);
    glm::vec3 sampleCoulombScattering(const Particle& particle, std::shared_ptr<Material> material);
    
    // Réduction de variance
    bool russianRoulette(Particle& particle);
    std::vector<Particle> splitting(const Particle& particle);
    
    // Optimisations
    float calculateImportance(const glm::vec3& position);
    void updateProgressiveResults();
    
    // Gestion d'erreurs
    void handleError(const std::string& message);
    
    // Sauvegarde/restauration d'état
    void saveCheckpoint(const std::string& filename);
    void loadCheckpoint(const std::string& filename);
};

// Solveur simplifié pour tests rapides
class SimplifiedSolver {
public:
    static float calculateAttenuationFactor(const glm::vec3& source, const glm::vec3& detector,
                                           RadiationType type, float energy, 
                                           std::shared_ptr<Scene> scene);
    
    static float calculateDoseRate(const glm::vec3& position, RadiationType type, 
                                 float sourceActivity, std::shared_ptr<Scene> scene);
    
    // Validation analytique
    static float analyticalAttenuation(float thickness, float mu);
    static float analyticalBuildup(float thickness, float mu, float energy);
};