#include <QObject>
#include "counter.hpp"

class App: public QObject {

    Q_OBJECT
    Q_PROPERTY(Counter *a READ a CONSTANT)
    Q_PROPERTY(Counter *b READ b CONSTANT)

public:
    App(QObject *parent = nullptr);
    Counter *a() { return &m_a; }
    Counter *b() { return &m_b; }

private:
    Counter m_a, m_b;
};