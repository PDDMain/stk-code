//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#include <iostream>
#include <sstream>
#include <assert.h>

#include "irrlicht.h"

#include "input/input.hpp"
#include "io/file_manager.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/modaldialog.hpp"
#include "guiengine/screen.hpp"
#include "guiengine/widget.hpp"
#include "utils/translation.hpp"

using namespace irr;

using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;
using namespace GUIEngine;

Screen::Screen(const char* file)
{
    m_mouse_x = 0;
    m_mouse_y = 0;
    this->m_filename = file;
    m_loaded = false;
    loadFromFile();
    m_inited = false;
}

#if 0
#pragma mark -
#pragma mark Load/Init
#endif

// -----------------------------------------------------------------------------
void Screen::loadFromFile()
{
    IrrXMLReader* xml = irr::io::createIrrXMLReader( (file_manager->getGUIDir() + "/" + m_filename).c_str() );
    parseScreenFileDiv(xml, m_widgets);
    m_loaded = true;
    calculateLayout();
}
// -----------------------------------------------------------------------------
/* small shortcut so this method can be called without arguments */
void Screen::calculateLayout()
{
    // build layout
    calculateLayout( m_widgets );
}
// -----------------------------------------------------------------------------
/*
 * Recursive call that lays out children widget within parent (or screen if none)
 * Manages 'horizontal-row' and 'vertical-row' layouts, along with the proportions
 * of the remaining children, as well as absolute sizes and locations.
 */
void Screen::calculateLayout(ptr_vector<Widget>& widgets, Widget* parent)
{
    const unsigned short widgets_amount = widgets.size();
    
    // ----- read x/y/size parameters
    for(unsigned short n=0; n<widgets_amount; n++)
    {
        widgets[n].readCoords(parent);
    }//next widget        
    
    // ----- manage 'layout's if relevant
    do // i'm using 'while false' here just to be able to 'break' ...
    {
        if(parent == NULL) break;
        
        std::string layout_name = parent->m_properties[PROP_LAYOUT];
        if(layout_name.size() < 1) break;
        
        bool horizontal = false;
        
        if (!strcmp("horizontal-row", layout_name.c_str()))
            horizontal = true;
        else if(!strcmp("vertical-row", layout_name.c_str()))
            horizontal = false;
        else
        {
            std::cerr << "Unknown layout name : " << layout_name.c_str() << std::endl;
            break;
        }
        
        const int w = parent->w, h = parent->h;
        
        // find space left after placing all absolutely-sized widgets in a row
        // (the space left will be divided between remaining widgets later)
        int left_space = (horizontal ? w : h);
        unsigned short total_proportion = 0;
        for(int n=0; n<widgets_amount; n++)
        {
            // relatively-sized widget
            std::string prop = widgets[n].m_properties[ PROP_PROPORTION ];
            if(prop.size() != 0)
            {
                total_proportion += atoi( prop.c_str() );
                continue;
            }
            
            // absolutely-sized widgets
            left_space -= (horizontal ? widgets[n].w : widgets[n].h);
        } // next widget
        
        // ---- lay widgets in row
        int x = parent->x, y = parent->y;
        for(int n=0; n<widgets_amount; n++)
        {
            std::string prop = widgets[n].m_properties[ PROP_PROPORTION ];
            if(prop.size() != 0)
            {
                // proportional size
                int proportion = 1;
                std::istringstream myStream(prop);
                if(!(myStream >> proportion))
                    std::cerr << "/!\\ Warning /!\\ : proportion  '" << prop.c_str() << "' is not a number in widget " << widgets[n].m_properties[PROP_ID].c_str() << std::endl;
                
                const float fraction = (float)proportion/(float)total_proportion;
                
                if(horizontal)
                {
   
                    widgets[n].x = x;
                    
                    if(widgets[n].m_properties[ PROP_Y ].size() > 0)
                        widgets[n].y = atoi(widgets[n].m_properties[ PROP_Y ].c_str());
                    else
                        widgets[n].y = y;
                    
                    widgets[n].w = (int)(left_space*fraction);
                    if(widgets[n].m_properties[PROP_MAX_WIDTH].size() > 0)
                    {
                        const int max_width = atoi( widgets[n].m_properties[PROP_MAX_WIDTH].c_str() );
                        if(widgets[n].w > max_width) widgets[n].w = max_width;
                    }
                    
                    x += widgets[n].w;
                }
                else
                {    
                    widgets[n].h = (int)(left_space*fraction);
                    
                    if(widgets[n].m_properties[PROP_MAX_HEIGHT].size() > 0)
                    {
                        const int max_height = atoi( widgets[n].m_properties[PROP_MAX_HEIGHT].c_str() );
                        if(widgets[n].h > max_height) widgets[n].h = max_height;
                    }
                    
                    std::string align = widgets[n].m_properties[ PROP_ALIGN ];
                    if(align.size() < 1)
                    {                        
                        if(widgets[n].m_properties[ PROP_X ].size() > 0)
                            widgets[n].x = atoi(widgets[n].m_properties[ PROP_X ].c_str());
                        else
                            widgets[n].x = x;
                    }
                    else if(align == "left") widgets[n].x = x;
                    else if(align == "center") widgets[n].x = x + w/2 - widgets[n].w/2;
                    else if(align == "right") widgets[n].x = x + w - widgets[n].w;
                    else std::cerr << "/!\\ Warning /!\\ : alignment  '" << align.c_str() << "' is unknown in widget " << widgets[n].m_properties[PROP_ID].c_str() << std::endl;
                    widgets[n].y = y;
                    
                    y += widgets[n].h;
                }
            }
            else
            {
                // absolute size
                
                if(horizontal)
                {
                    widgets[n].x = x;
                    
                    if(widgets[n].m_properties[ PROP_Y ].size() > 0)
                        widgets[n].y = atoi(widgets[n].m_properties[ PROP_Y ].c_str());
                    else
                        widgets[n].y = y;
                    
                    x += widgets[n].w;
                }
                else
                {
                    //widgets[n].h = abs_var;
                    
                    std::string align = widgets[n].m_properties[ PROP_ALIGN ];
                    
                    if(align.size() < 1)
                    {
                        if(widgets[n].m_properties[ PROP_X ].size() > 0)
                            widgets[n].x = atoi(widgets[n].m_properties[ PROP_X ].c_str());
                        else
                            widgets[n].x = x;
                    }
                    else if(align == "left") widgets[n].x = x;
                    else if(align == "center") widgets[n].x = x + w/2 - widgets[n].w/2;
                    else if(align == "right") widgets[n].x = x + w - widgets[n].w;
                    else std::cerr << "/!\\ Warning /!\\ : alignment  '" << align.c_str() << "' is unknown in widget " << widgets[n].m_properties[PROP_ID].c_str() << std::endl;
                    widgets[n].y = y;
                    
                    y += widgets[n].h;
                }
            } // end if property or absolute size
            
        } // next widget
        
    } while(false);
    
    // ----- also deal with containers' children
    for(int n=0; n<widgets_amount; n++)
    {
        if(widgets[n].m_type == WTYPE_DIV) calculateLayout(widgets[n].m_children, &widgets[n]);
    }
}

// -----------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Adding/Removing widgets
#endif


void Screen::addWidgets()
{
    if (!m_loaded) loadFromFile();
    
    addWidgetsRecursively( m_widgets );

    // select the first widget
    Widget* w = getFirstWidget();
    if(w != NULL) GUIEngine::getGUIEnv()->setFocus( w->m_element );
}
// -----------------------------------------------------------------------------
void Screen::addWidgetsRecursively(ptr_vector<Widget>& widgets, Widget* parent)
{
    const unsigned short widgets_amount = widgets.size();
    
    // ------- add widgets
    for (int n=0; n<widgets_amount; n++)
    {
        if (widgets[n].m_type == WTYPE_DIV)
        {
            widgets[n].add(); // Will do nothing, but will maybe reserve an ID
            addWidgetsRecursively(widgets[n].m_children, &widgets[n]);
        }
        else
        {
            // warn if widget has no dimensions (except for ribbons and icons, where it is normal since it adjusts to its contents)
            if((widgets[n].w < 1 || widgets[n].h < 1) && widgets[n].m_type != WTYPE_RIBBON  && widgets[n].m_type != WTYPE_ICON_BUTTON)
                std::cerr << "/!\\ Warning /!\\ : widget " << widgets[n].m_properties[PROP_ID].c_str() << " has no dimensions" << std::endl;
            
            if(widgets[n].x == -1 || widgets[n].y == -1)
                std::cerr << "/!\\ Warning /!\\ : widget " << widgets[n].m_properties[PROP_ID].c_str() << " has no position" << std::endl;
            
            widgets[n].add();
        }
        
    } // next widget
    
}

// -----------------------------------------------------------------------------
/**
 * Called when screen is removed. This means all irrlicht widgets this object has pointers
 * to are now gone. Set all references to NULL to avoid problems.
 */
void Screen::elementsWereDeleted(ptr_vector<Widget>* within_vector)
{
    if(within_vector == NULL) within_vector = &m_widgets;
    const unsigned short widgets_amount = within_vector->size();
    
    for(int n=0; n<widgets_amount; n++)
    {
        Widget& widget = (*within_vector)[n];
        
        widget.m_element = NULL;
        
        if(widget.m_children.size() > 0)
        {
            elementsWereDeleted( &(widget.m_children) );
        }
    }
}
// -----------------------------------------------------------------------------
void Screen::manualAddWidget(Widget* w)
{
    m_widgets.push_back(w);
}
// -----------------------------------------------------------------------------
void Screen::manualRemoveWidget(Widget* w)
{
    m_widgets.remove(w);
}

// -----------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Getting widgets
#endif

Widget* Screen::getWidget(const char* name)
{
    return getWidget(name, &m_widgets);
}
// -----------------------------------------------------------------------------
Widget* Screen::getWidget(const char* name, ptr_vector<Widget>* within_vector)
{
    const unsigned short widgets_amount = within_vector->size();
    
    for(int n=0; n<widgets_amount; n++)
    {
        Widget& widget = (*within_vector)[n];
        
        if(widget.m_properties[PROP_ID] == name) return &widget;
        
        if(widget.m_type == WTYPE_DIV)
        {
            Widget* el = getWidget(name, &(widget.m_children));
            if(el != NULL) return el;
        }
    } // next
    
    return NULL;
}
// -----------------------------------------------------------------------------
Widget* Screen::getWidget(const int id, ptr_vector<Widget>* within_vector)
{
    const unsigned short widgets_amount = within_vector->size();
    
    for(int n=0; n<widgets_amount; n++)
    {
        Widget& widget = (*within_vector)[n];
        
        if(widget.m_element != NULL && widget.m_element->getID() == id) return &widget;
        
        if(widget.m_children.size() > 0)
        {
            // std::cout << "widget = <" << widget.m_properties[PROP_ID].c_str() << ">  widget.m_children.size()=" << widget.m_children.size() << std::endl;
            Widget* el = getWidget(id, &(widget.m_children));
            if(el != NULL) return el;
        }
    } // next
    
    return NULL;
}
// -----------------------------------------------------------------------------
Widget* Screen::getFirstWidget(ptr_vector<Widget>* within_vector)
{
    if(within_vector == NULL) within_vector = &m_widgets;
    
    for(int i = 0; i < within_vector->size(); i++)
    {
        // if container, also checks children
        if(within_vector->get(i)->m_children.size() > 0 &&
           within_vector->get(i)->m_type != WTYPE_RIBBON &&
           within_vector->get(i)->m_type != WTYPE_SPINNER)
        {
            Widget* w = getFirstWidget(&within_vector->get(i)->m_children);
            if(w != NULL) return w;
        }
        
        if(within_vector->get(i)->m_element == NULL || within_vector->get(i)->m_element->getTabOrder() == -1) continue;
        
        return within_vector->get(i);
    }
    return NULL;
}
// -----------------------------------------------------------------------------
Widget* Screen::getLastWidget(ptr_vector<Widget>* within_vector)
{
    if (within_vector == NULL) within_vector = &m_widgets;
    
    for(int i = within_vector->size()-1; i >= 0; i--)
    {
        // if container, also checks children
        if(within_vector->get(i)->m_children.size() > 0 &&
           within_vector->get(i)->m_type != WTYPE_RIBBON &&
           within_vector->get(i)->m_type != WTYPE_SPINNER)
        {
            Widget* w = getLastWidget(&within_vector->get(i)->m_children);
            if(w != NULL) return w;
        }
        
        if(within_vector->get(i)->m_element == NULL || within_vector->get(i)->m_element->getTabOrder() == -1) continue;
        
        return within_vector->get(i);
    }
    return NULL;
}


