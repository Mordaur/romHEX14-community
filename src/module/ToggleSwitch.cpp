/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ToggleSwitch.h"
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QMouseEvent>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
}

QSize ToggleSwitch::sizeHint() const
{
    // Scales with the widget font so it grows with the interface-scale setting.
    const int h = qMax(22, fontMetrics().height() + 6);
    return QSize(h * 2, h);
}

void ToggleSwitch::setChecked(bool on, bool animate)
{
    m_checked = on;
    if (animate) animateTo(on ? 1.0 : 0.0);
    else { m_pos = on ? 1.0 : 0.0; update(); }
}

void ToggleSwitch::animateTo(qreal target)
{
    auto *a = new QPropertyAnimation(this, "position", this);
    a->setDuration(160);
    a->setStartValue(m_pos);
    a->setEndValue(target);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
        m_checked = !m_checked;
        animateTo(m_checked ? 1.0 : 0.0);
        emit toggled(m_checked);
    }
}

void ToggleSwitch::enterEvent(QEnterEvent *) { m_hover = true;  update(); }
void ToggleSwitch::leaveEvent(QEvent *)      { m_hover = false; update(); }

static QColor lerp(const QColor &a, const QColor &b, qreal t)
{
    return QColor(int(a.red()   + (b.red()   - a.red())   * t),
                  int(a.green() + (b.green() - a.green()) * t),
                  int(a.blue()  + (b.blue()  - a.blue())  * t));
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal h = height();
    const qreal w = width();
    const qreal r = h / 2.0;
    const QRectF track(0, 0, w, h);

    // Glow behind the track when on.
    if (m_pos > 0.02) {
        QColor glow = m_onColor;
        glow.setAlphaF(0.35 * m_pos);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawRoundedRect(track.adjusted(-3, -3, 3, 3), r + 3, r + 3);
    }

    // Track — interpolate off→on colour.
    QColor track_c = lerp(m_offColor, m_onColor, m_pos);
    p.setPen(Qt::NoPen);
    p.setBrush(track_c);
    p.drawRoundedRect(track, r, r);

    // Inner subtle top-light gradient for depth.
    QLinearGradient g(0, 0, 0, h);
    g.setColorAt(0.0, QColor(255, 255, 255, 26));
    g.setColorAt(1.0, QColor(0, 0, 0, 30));
    p.setBrush(g);
    p.drawRoundedRect(track, r, r);

    // Knob.
    const qreal m = h * 0.12;                 // margin
    const qreal kd = h - 2 * m;               // knob diameter
    const qreal kx = m + m_pos * (w - 2 * m - kd);
    const QRectF knob(kx, m, kd, kd);

    // knob shadow
    p.setBrush(QColor(0, 0, 0, 60));
    p.drawEllipse(knob.translated(0, 1.2));
    // knob body
    QRadialGradient kg(knob.center(), kd / 2.0);
    kg.setColorAt(0.0, QColor(255, 255, 255));
    kg.setColorAt(1.0, QColor(226, 232, 240));
    p.setBrush(kg);
    p.drawEllipse(knob);

    if (m_hover) {
        p.setPen(QPen(QColor(255, 255, 255, 60), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(track.adjusted(0.5, 0.5, -0.5, -0.5), r, r);
    }
}
