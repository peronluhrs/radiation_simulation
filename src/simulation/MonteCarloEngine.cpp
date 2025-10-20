#include "utils/Random.h"
#include "simulation/MonteCarloEngine.h"
#include "simulation/Particle.h"
#include <algorithm>
#include <future>

// Générateur aléatoire thread-local pour le moteur Monte Carlo
thread_local std::mt19937 MonteCarloEngine::s_rng(std::random_device{}());

MonteCarloEngine::MonteCarloEngine(std::shared_ptr<Scene> scene)
    : m_scene(scene)
{
}

MonteCarloEngine::~MonteCarloEngine()
{
    stopSimulation();
}

void MonteCarloEngine::startSimulation()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (m_state == SimulationState::RUNNING)
        return;

    m_shouldStop = false;
    m_shouldPause = false;
    m_state = SimulationState::RUNNING;
    m_stats.startTime = std::chrono::steady_clock::now();

    // Lancement des threads de travail
    m_workers.clear();
    for (uint32_t i = 0; i < m_config.numThreads; ++i)
    {
        m_workers.emplace_back(&MonteCarloEngine::workerThread, this, i);
    }

    Log::info("Simulation Monte Carlo démarrée avec " +
              std::to_string(m_config.numThreads) + " threads");
}

void MonteCarloEngine::stopSimulation()
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_shouldStop = true;
        m_shouldPause = false;
        m_state = SimulationState::IDLE;
    }

    m_pauseCondition.notify_all();

    // Attendre la fin de tous les threads
    for (auto &worker : m_workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    m_workers.clear();

    m_stats.endTime = std::chrono::steady_clock::now();
    Log::info("Simulation arrêtée");
}

void MonteCarloEngine::pauseSimulation()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_state == SimulationState::RUNNING)
    {
        m_shouldPause = true;
        m_state = SimulationState::PAUSED;
    }
}

void MonteCarloEngine::resumeSimulation()
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_state == SimulationState::PAUSED)
        {
            m_shouldPause = false;
            m_state = SimulationState::RUNNING;
        }
    }
    m_pauseCondition.notify_all();
}

void MonteCarloEngine::runBatch(uint32_t numParticles)
{
    if (!m_scene)
        return;

    auto sources = m_scene->getAllSources();
    if (sources.empty())
        return;

    std::uniform_int_distribution<size_t> sourceDist(0, sources.size() - 1);

    for (uint32_t i = 0; i < numParticles; ++i)
    {
        // Sélection aléatoire d'une source
        auto source = sources[sourceDist(s_rng)];
        if (!source->isEnabled())
            continue;

        // Émission d'une particule
        Particle particle = source->emitParticle();
        m_stats.particlesEmitted.fetch_add(1);

        // Transport
        transportParticleInternal(particle);
    }
}

float MonteCarloEngine::getProgress() const
{
    uint64_t emitted = m_stats.particlesEmitted.load();
    return std::min(1.0f, static_cast<float>(emitted) / m_config.maxParticles);
}

void MonteCarloEngine::workerThread(uint32_t threadId)
{
    const uint32_t batchSize = 1000;

    while (!m_shouldStop)
    {
        // Vérification de pause
        {
            std::unique_lock<std::mutex> lock(m_stateMutex);
            m_pauseCondition.wait(lock, [this]
                                  { return !m_shouldPause || m_shouldStop; });

            if (m_shouldStop)
                break;
        }

        // Vérification si on a atteint la limite
        if (m_stats.particlesEmitted.load() >= m_config.maxParticles)
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_state = SimulationState::COMPLETED;
            break;
        }

        // Traitement d'un batch
        emitAndTransportBatch(batchSize, threadId);
    }
}

void MonteCarloEngine::emitAndTransportBatch(uint32_t batchSize, uint32_t threadId)
{
    if (!m_scene)
        return;

    auto sources = m_scene->getAllSources();
    if (sources.empty())
        return;

    std::uniform_int_distribution<size_t> sourceDist(0, sources.size() - 1);

    for (uint32_t i = 0; i < batchSize && !m_shouldStop; ++i)
    {
        if (m_stats.particlesEmitted.load() >= m_config.maxParticles)
            break;

        // Sélection d'une source active
        auto source = sources[sourceDist(s_rng)];
        if (!source->isEnabled())
            continue;

        // Émission
        Particle particle = source->emitParticle();
        source->incrementEmitted();
        m_stats.particlesEmitted.fetch_add(1);

        // Transport
        transportParticleInternal(particle);
    }
}

void MonteCarloEngine::transportParticle(Particle &particle)
{
    transportParticleInternal(particle);
}

void MonteCarloEngine::transportParticleInternal(Particle &particle)
{
    m_stats.particlesTransported.fetch_add(1);

    uint32_t bounceCount = 0;

    while (particle.isActive() && bounceCount < m_config.maxBounces)
    {
        if (m_shouldStop)
            break;

        // Énergie minimale
        if (particle.getEnergy() < m_config.energyCutoff)
        {
            particle.absorb();
            break;
        }

        // Temps maximum
        if (particle.getAge() > m_config.timeCutoff)
        {
            particle.escape();
            break;
        }

        // Étape de transport
        if (!stepParticle(particle))
        {
            break;
        }

        ++bounceCount;

        // Roulette russe pour terminer les particules de faible poids
        if (m_config.useRussianRoulette && particle.getWeight() < m_config.russianRouletteThreshold)
        {
            if (!russianRoulette(particle))
            {
                break;
            }
        }
    }

    // Statistiques finales
    switch (particle.getState())
    {
    case ParticleState::ABSORBED:
        m_stats.particlesAbsorbed.fetch_add(1);
        break;
    case ParticleState::DETECTED:
        m_stats.particlesDetected.fetch_add(1);
        break;
    case ParticleState::ESCAPED:
        m_stats.particlesEscaped.fetch_add(1);
        break;
    default:
        break;
    }
}

bool MonteCarloEngine::stepParticle(Particle &particle)
{
    // Intersection avec la géométrie
    Ray ray = particle.getRay();
    IntersectionResult hit = m_scene->intersectRay(ray);
    m_stats.rayIntersections.fetch_add(1);

    if (!hit.hit)
    {
        // Pas d'intersection - particule s'échappe
        particle.escape();
        return false;
    }

    // Déplacement jusqu'au point d'intersection
    particle.move(hit.distance);
    particle.setCurrentMaterial(hit.material);

    // Vérification des capteurs le long du trajet
    auto sensors = m_scene->getAllSensors();
    for (auto &sensor : sensors)
    {
        if (sensor->detectsParticle(particle))
        {
            sensor->recordDetection(particle);
            if (particle.getState() == ParticleState::DETECTED)
            {
                return false;
            }
        }
    }

    // Interaction avec le matériau
    if (hit.material)
    {
        InteractionType interaction = sampleInteraction(particle, hit.material);
        processInteraction(particle, interaction, hit.material);
        m_stats.totalCollisions.fetch_add(1);
    }

    return particle.isActive();
}

InteractionType MonteCarloEngine::sampleInteraction(const Particle &particle,
                                                    std::shared_ptr<Material> material)
{
    return material->sampleInteraction(particle.getType(), particle.getEnergy());
}

void MonteCarloEngine::processInteraction(Particle &particle, InteractionType interaction,
                                          std::shared_ptr<Material> material)
{
    switch (interaction)
    {
    case InteractionType::ABSORPTION:
        particle.absorb();
        break;

    case InteractionType::SCATTERING:
    {
        glm::vec3 newDir = material->sampleScattering(particle.getDirection(),
                                                      particle.getType(),
                                                      particle.getEnergy());

        // Perte d'énergie (simplifiée)
        float energyLoss = 0.1f * particle.getEnergy() * RandomGenerator::random();
        particle.scatter(newDir, energyLoss);
        break;
    }

    case InteractionType::CAPTURE:
        particle.absorb();
        break;

    case InteractionType::TRANSMISSION:
        // Pas d'interaction - continue
        break;
    }
}

bool MonteCarloEngine::russianRoulette(Particle &particle)
{
    float thr = std::max(1e-6f, m_config.russianRouletteThreshold);
    float survivalProb = std::min(1.0f, particle.getWeight() / thr);
    float r = RandomGenerator::random();
    if (r < survivalProb)
    {
        particle.setWeight(particle.getWeight() / survivalProb);
        return false; // continue
    }
    else
    {
        particle.absorb();
        return true; // stop
    }
}

std::vector<Particle> MonteCarloEngine::splitting(const Particle &particle)
{
    std::vector<Particle> split;

    float newWeight = particle.getWeight() / m_config.splittingFactor;

    for (uint32_t i = 0; i < m_config.splittingFactor; ++i)
    {
        Particle newParticle = particle;
        newParticle.setWeight(newWeight);
        newParticle.setGeneration(particle.getGeneration() + 1);
        split.push_back(newParticle);
    }

    return split;
}

void MonteCarloEngine::handleError(const std::string &message)
{
    Log::error("Erreur simulation: " + message);
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_state = SimulationState::ERROR;
}

// SimplifiedSolver
float SimplifiedSolver::calculateAttenuationFactor(const glm::vec3 &source,
                                                   const glm::vec3 &detector,
                                                   RadiationType type, float energy,
                                                   std::shared_ptr<Scene> scene)
{
    Ray ray(source, glm::normalize(detector - source));
    float totalAttenuation = 0.0f;
    float distance = glm::length(detector - source);

    // Échantillonnage le long du rayon
    const int numSamples = 100;
    float stepSize = distance / numSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        glm::vec3 pos = source + ray.direction * (i * stepSize);
        Ray sampleRay(pos, ray.direction);
        sampleRay.tMax = stepSize;

        IntersectionResult hit = scene->intersectRay(sampleRay);
        if (hit.hit && hit.material)
        {
            float mu = hit.material->getLinearAttenuation(type, energy);
            totalAttenuation += mu * stepSize;
        }
    }

    return std::exp(-totalAttenuation);
}

float SimplifiedSolver::analyticalAttenuation(float thickness, float mu)
{
    return std::exp(-mu * thickness);
}