#ifndef PARTICICLESYSTEM_H_
#define PARTICICLESYSTEM_H_

#include "Object.h"

class ParticleSystem : public sf::Transformable, public sf::Drawable
{
public:
    ParticleSystem(unsigned int count = 20000) 
    : m_particles(count), m_vertices(count), m_emitter(0.f, 0.f), vertexBuffer(sf::PrimitiveType::Points, sf::VertexBuffer::Stream)
    {
        for (auto &vertex : m_vertices)
            vertex.color = sf::Color::Transparent;
        for (auto &particicle : m_particles)
            particicle.lifetime = 0.f;
        vertexBuffer.create(count);
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
        vertexBuffer.update(m_vertices.data());
    }
    void impulse(sf::Vector2f position, uint8_t explosion = 1)
    {
        setEmitter(position);
        std::size_t j = 0;
        for (std::size_t i = 0; i < m_particles.size(); i++)
        {
            if (m_particles[i].lifetime < 0.f)
            {
                if (explosion > 1)
                    resetParticle(i, float(explosion - 2) * 2.f * pi / 253.f + Object::normalRNG<0, 1, 1, 10>());
                else
                    resetParticle(i);
                j++;
            }
            if (explosion == 1 && j >= 300)
                break;
            else if (j >= 600)
                break;
        }
    }

private:
    struct Particle
    {
        sf::Vector2f velocity;
        float lifetime;
    };
    void setEmitter(const sf::Vector2f &position)
    {
        m_emitter = position;
    }
    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const noexcept override
    {
        target.draw(vertexBuffer);
    }
    void resetParticle(std::size_t index, float angle = Object::uniformRNG<0, 1, 1, 1>() * pi * 2.f)
    {
        float speed = Object::uniformRNG<0, 1, 1, 1>() * 145.f + 5.f;
        m_particles[index].velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
        m_particles[index].lifetime = Object::uniformRNG<0, 1, 1, 1>() * 2.7f + 0.3f;
        m_vertices[index].position = m_emitter;
        m_vertices[index].color = sf::Color::White;
    }

    sf::VertexBuffer vertexBuffer;
    std::vector<Particle> m_particles;
    std::vector<sf::Vertex> m_vertices;
    sf::Vector2f m_emitter;
};

#endif
