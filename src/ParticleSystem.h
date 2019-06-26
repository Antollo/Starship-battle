#ifndef PARTICICLESYSTEM_H_
#define PARTICICLESYSTEM_H_

#include "Object.h"

class ParticleSystem : public sf::Transformable, public sf::Drawable
{
public:
    /*static ParticleSystem* create()
    {
        particleSystem = dynamic_cast<ParticleSystem*>(objects.emplace(counter, new ParticleSystem()).first->second.get());
        return particleSystem;
    }*/
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
    /*const TypeId getTypeId() const noexcept override
    {
        return Object::TypeId::ParticicleSystem;
    }*/

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
        float angle  = Object::rng01(Object::mt) * pi * 2.f;
        float speed = Object::rng01(Object::mt) * 150.f;
        m_particles[index].velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
        m_particles[index].lifetime = Object::rng01(Object::mt) * 2.5f;
        m_vertices[index].position = m_emitter;
        m_vertices[index].color = sf::Color::White;
    }

    std::vector<Particle> m_particles;
    std::vector<sf::Vertex> m_vertices;
    sf::Vector2f m_emitter;
};

#endif

