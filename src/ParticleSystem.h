#ifndef PARTICICLESYSTEM_H_
#define PARTICICLESYSTEM_H_

#include "Object.h"

class ParticleSystem : public sf::Transformable, public Object
{
public:
    static ParticleSystem* create()
    {
        objects.emplace_back(new ParticleSystem());
        particleSystem = dynamic_cast<ParticleSystem*>(objects.front().get());
        return particleSystem;
    }
    ParticleSystem(unsigned int count = 10000) :
    m_particles(count),
    m_vertices(count),
    m_emitter(0.f, 0.f)
    {
        for (auto& vertex : m_vertices)
            vertex.color = sf::Color::Transparent;
        for (auto& particicle : m_particles)
            particicle.lifetime = 0.f;
    }
    void update(float elapsed)
    {
        for (std::size_t i = 0; i < m_particles.size(); i++)
        {
            m_particles[i].lifetime -= elapsed;
            if (m_particles[i].lifetime < 0.f)
                m_vertices[i].color = sf::Color::Transparent;
            m_vertices[i].position += m_particles[i].velocity * elapsed;
        }
    }
    void impulse(sf::Vector2f position)
    {
        setEmitter(position);
        std::size_t j = 0;
        for (std::size_t i = 0; i < m_particles.size(); i++)
        {
            if (m_particles[i].lifetime < 0.f)
            {
                resetParticle(i);
                j++;
            }
            if (j >= 300) break;
        }
    }
    const typeId getTypeId() const noexcept override
    {
        return Object::typeId::ParticicleSystem;
    }

private:
    struct Particle
    {
        sf::Vector2f velocity;
        float lifetime;
    };
    void setEmitter(const sf::Vector2f& position)
    {
        m_emitter = position;
    }
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        states.texture = nullptr;
        target.draw(&m_vertices[0], m_vertices.size(), sf::Points, states);
    }
    void resetParticle(std::size_t index)
    {
        float angle  = rng01(mt) * pi * 2.f;
        float speed = rng01(mt) * 150.f;
        m_particles[index].velocity = sf::Vector2f(std::cosf(angle) * speed, std::sinf(angle) * speed);
        m_particles[index].lifetime = rng01(mt) * 2.5f;
        m_vertices[index].position = m_emitter;
        m_vertices[index].color = sf::Color::White;
    }

    std::vector<Particle> m_particles;
    std::vector<sf::Vertex> m_vertices;
    sf::Vector2f m_emitter;
};

#endif PARTICICLESYSTEM_H_