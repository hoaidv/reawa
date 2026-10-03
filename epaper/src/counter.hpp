#pragma once 
#include <QObject>


class Counter: public QObject {

    Q_OBJECT 
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit Counter(QObject *parent = nullptr): QObject(parent) {
        
    }

    int value() {
        return m_value;
    }

public slots:
    void setValue(int value) {
        if (value != m_value) {
            m_value = value;
            emit valueChanged(value);
        }
    }

signals:

    void valueChanged(int newValue);

private:
    int m_value = 0;
};