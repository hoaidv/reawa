#include "app.hpp"

App::App(QObject *parent) : QObject(parent), m_a(this), m_b(this) {
    
    m_b.setValue(m_a.value() * 2);

    connect(&m_a, &Counter::valueChanged, this, [this](int value) {
        m_b.setValue(value * 2);
    });
}
