/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                                                   *
 *  WONDER - Wave field synthesis Of New Dimensions of Electronic music in Realtime  *
 *  http://swonder.sourceforge.net                                                   *
 *                                                                                   *
 *                                                                                   *
 *  Technische Universität Berlin, Germany                                           *
 *  Audio Communication Group                                                        *
 *  www.ak.tu-berlin.de                                                              *
 *  Copyright 2006-2008                                                              *
 *                                                                                   *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or modify             *
 *  it under the terms of the GNU General Public License as published by             *
 *  the Free Software Foundation; either version 2 of the License, or                *
 *  (at your option) any later version.                                              *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.       *
 *                                                                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef QFWONDER_H
#define QFWONDER_H

#include <QWidget>

class QPushButton;
class QLabel;
class QSlider;
class CacheMatrix;
class QVBoxLayout;
class QHBoxLayout;


class QFWonder : public QWidget
{
    Q_OBJECT

public:
    QFWonder( QWidget* parent = 0 );

private slots:
    void currentIR( int x, int y );
    void reset();

private:
    QVBoxLayout* mainLayout;
    QHBoxLayout* labelLayout;

    QLabel* currentIRLabel;
    QLabel* currentIRValueLabel;
    QLabel* numStaticIRsLoadedLabel;
    QLabel* numStaticIRsLoadedValueLabel;
    QLabel* numDynamicIRsLoadedLabel;
    QLabel* numDynamicIRsLoadedValueLabel;

    CacheMatrix* cacheMatrix;
    QSlider*     sizeSlider;
};

#endif // QFWONDER_H
