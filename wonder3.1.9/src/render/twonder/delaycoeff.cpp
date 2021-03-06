/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                                                   *
 *  WONDER - Wave field synthesis Of New Dimensions of Electronic music in Realtime  *
 *  http://swonder.sourceforge.net                                                   *
 *                                                                                   *
 *  Torben Hohn, Eddie Mond, Marije Baalman                                          *
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

#include "delaycoeff.h"

unsigned int DelayCoeff::getSampleDelayRounded( float preDelay )
{
    if( delay + preDelay < 0 )
        return 0;
    else
        return ( unsigned int ) ( ( delay + preDelay ) * ( twonderConf->sampleRate / twonderConf->soundSpeed ) + 0.5 ); 
		/// NOTE: why is there a 0.5 here?? ensuring it is always >= 1?
		/// NOTE: original source uses lrintf (is that an optimized cast?)
}


float DelayCoeff::getSampleDelay( float preDelay )
{
    if( delay + preDelay < 0 )
        return 0.;
    else
        return  ( delay + preDelay ) * ( twonderConf->sampleRate / twonderConf->soundSpeed );
}


void DelayCoeff::print() 
{ 
    std::cout << "delay: "               << getDelay()
              << " [m] | SampleDelay = " << getSampleDelay( twonderConf->negDelayInit ) 
              << " | Amplitudefactor: "  << getFactor() 
              << std::endl;
}
