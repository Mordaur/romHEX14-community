/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>
#include <QColor>

// A custom-painted iOS/automotive-style toggle: a sliding, glowing knob on a
// rounded track. Used for the BCM VTS control. setChecked() moves it silently
// (for reflecting real state); a user click animates it and emits toggled().
class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition)
public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    bool isChecked() const { return m_checked; }
    void setChecked(bool on, bool animate = true);   // no signal
    void setOnColor(const QColor &c) { m_onColor = c; update(); }

    qreal position() const { return m_pos; }
    void  setPosition(qreal p) { m_pos = p; update(); }

    QSize sizeHint() const override;

signals:
    void toggled(bool checked);   // emitted only on user interaction

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    void animateTo(qreal target);

    bool   m_checked = false;
    qreal  m_pos     = 0.0;      // 0 = off (left), 1 = on (right)
    bool   m_hover   = false;
    QColor m_onColor = QColor(0x3f, 0xb9, 0x50);
    QColor m_offColor = QColor(0x3a, 0x3f, 0x47);
};
