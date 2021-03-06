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

#include "project.h"
#include "wonder_path.h"
#include <exception>
#include <iostream>
#include <sstream>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::exception;
using std::list;

using xmlpp::Attribute;
using xmlpp::NodeSet;
using xmlpp::DtdValidator;
using xmlpp::Document;
using xmlpp::Node;
using xmlpp::Element;


//---------------------------------Project------------------------------------//

Project::Project( int maxNoSources )
{   
    fromFile = true;
    
    rootNode = NULL;
    doc      = NULL;
    scenario = NULL;
    dtdFile  = "cwonder_project.dtd";
    dtdPath  = "";

    name          = "";
    scoreFileName = "";

    this->maxNoSources = maxNoSources;
}


Project::~Project()
{
    if( ! fromFile )
    {
        if( doc )
        {
            delete doc;
            doc = NULL;
        }
    }
    clearSnaphots();
}


void Project::createProject( bool withScore )
{
    if( fromFile )
    {
        doc      = new Document;
        fromFile = false; // flag for "must be deleted at the end"
    }

    clearSnaphots();

    if( withScore )
        scoreFileName = name + "_score";
    else
        scoreFileName = "";

    ostringstream os;
    doc->set_internal_subset( "cwonder_project", "", dtdFile );
    rootNode = doc->create_root_node( "cwonder_project");
    rootNode->set_attribute( "name", name );
    rootNode->set_attribute( "scorefile", scoreFileName );

    createScenario();
}

int Project::addScore()
{
	if ( scoreFileName != "" ){
		scoreFileName = name + "_score";
    	rootNode->set_attribute( "scorefile", scoreFileName );
		return 0;
	}
	return -1;
}

int Project::createScenario()
{
    // this can only be done once, so if data in snapshots is
    // there, this call is not allowed
    if( ! snapshots.empty() )
        return -1;

    Scenario* newSnapshot = new Scenario( maxNoSources );
    newSnapshot->id = 0; // 0 is default, but just to make sure

    if( rootNode )
    {
        Element* snapshotElement = NULL;
        snapshotElement   = rootNode->add_child( "scenario" );
        newSnapshot->node = snapshotElement;

        // Validate and add snapshot to the list of snapshots
        validate();
        snapshots.push_back( newSnapshot );
        scenario = newSnapshot;

        return 0;
    }
    else
    {
        delete newSnapshot;
    }

    return 1; // cannot get root node
}


Scenario* Project::getScenario()
{
    if( ! snapshots.empty() )
        return snapshots.front();
    else
        return NULL;
}


void Project::writeProjectToDOM()
{
    try
    {
        Element::AttributeList::const_iterator attribIter;
        if( const Element* nodeElement = dynamic_cast< const Element* >( rootNode ) ) 
        {               
            // write header
            const Element::AttributeList& attributes = nodeElement->get_attributes();
            for( attribIter = attributes.begin(); attribIter != attributes.end(); ++attribIter )
            {
                Attribute* attribute = *attribIter;
                attribName = attribute->get_name();             
                ostringstream os;

                if ( attribName == "name" )
                    os << name;
                else if ( attribName == "scorefile" )
                    os << scoreFileName;
                
                const Glib::ustring out = os.str();
                attribute->set_value( out );
            }   
            // write snapshots
            writeSnapshotsToDOM();
        }
    }
    catch( const exception& ex )
    {
        cout << "Exception caught: " << ex.what() << endl;
    }
}


void Project::readProjectFromDOM()
{
    name          = "";
    scoreFileName = "";

    if ( rootNode )
    {
        const Element::AttributeList& attributes = rootNode->get_attributes();

        Element::AttributeList::const_iterator attribIter;
        for( attribIter = attributes.begin(); attribIter!= attributes.end(); ++attribIter )
        {
            const Attribute* projectAttribute = *attribIter;
            Glib::ustring attribName          = projectAttribute->get_name();

            if ( attribName == "name" )
                name = projectAttribute->get_value();
            else if ( attribName == "scorefile" )
                scoreFileName = projectAttribute->get_value();
        }
    }
    try
    {
        readSnapshotsFromDOM( rootNode, "/cwonder_project/scenario" );
        readSnapshotsFromDOM( rootNode, "/cwonder_project/snapshot" );
    }
    catch( exception& ex )
    {
        // because readSnapshotsFromDOM allocates memory we have to free 
        // that first and then rethrow this exception
        clearSnaphots();
        if( doc )
        {
            doc      = NULL;
            rootNode = NULL;
        } 
        throw;
    }
    scenario = snapshots.front();
}


void Project::clearSnaphots()
{   
    // this clears the scenario as well
    while ( ! snapshots.empty() )
    {
        delete snapshots.back(); 
        snapshots.pop_back();    
    }
}


void Project::readFromFile( Glib::ustring filepath )
{
    clearSnaphots();

    // Read the Dom representation from an xml-file
    parser.set_substitute_entities( false );
    parser.parse_file( filepath );
    if( parser )
    {
        rootNode = parser.get_document()->get_root_node();
        if( rootNode != NULL)
        {
            if( fromFile == false )
                delete doc;
            
            fromFile = true;
            doc      = parser.get_document();        
            validate();

            // remove all textnodes from the complete xml tree to get
            // nice linebreaks when writing to file with write_to_file_formatted()
            Node::NodeList snapshotNodeList = rootNode->get_children();
            Node::NodeList::iterator it;
            for( it = snapshotNodeList.begin(); it != snapshotNodeList.end(); ++it )
            {
                Node* snapshotNode = *it;
                if( snapshotNode->get_name() == "text")
                    rootNode->remove_child( snapshotNode );
                else
                {
                    Node::NodeList childNodeList = snapshotNode->get_children();
                    Node::NodeList::iterator it;
                    for( it = childNodeList.begin(); it != childNodeList.end(); ++it )
                    {
                        Node* sourceOrGroupNode = *it;
                        if( sourceOrGroupNode->get_name() == "text" )
                            snapshotNode->remove_child( sourceOrGroupNode );
                    }
                }
            }
        }
    }
    readProjectFromDOM();
}


void Project::writeToFile( string filename )
{
    writeProjectToDOM();

    if( doc )   
        doc->write_to_file_formatted( filename.c_str() );    
}


string Project::show()
{
    if( doc )
    {
        writeProjectToDOM();
    
        // return complete DOM representation without snapshot source data 
        Document shortDoc;
        shortDoc.create_root_node_by_import( rootNode );
        Element*       shortRootNode = shortDoc.get_root_node();
        Node::NodeList snapshotList  = shortRootNode->get_children();

        for( Node::NodeList::iterator iter = snapshotList.begin(); iter != snapshotList.end(); ++iter)
        {
            if( Element* nodeElement = dynamic_cast< Element* > ( *iter ) ) 
            {
                if( nodeElement->get_name() == "snapshot" )
                {
                    Node::NodeList childList; 
                    childList =  nodeElement->get_children();

                    for( Node::NodeList::iterator srcIt = childList.begin(); srcIt != childList.end(); ++srcIt )
                        nodeElement->remove_child( *srcIt );
                }
            }
        }

        return shortDoc.write_to_string_formatted();
    }
    return "";
}


int Project::validate()
{
    DtdValidator validator( join( dtdPath, dtdFile ) );
    validator.validate( doc );
    return 0;
}


int Project::takeSnapshot( int snapshotID, string name )
{
    if( snapshots.empty() )
        return -1; // no scenario

    if( snapshotID <= 0 )
        return -1; // invalid snapshotID

    // check if snapshot already exists if so overwrite existing snapshot
    // otherwise create a new one
    bool overwrite = false;
    list< Scenario* >::iterator iter;
    for( iter = snapshots.begin(); iter != snapshots.end(); ++iter )
    {
        if(  ( *iter )->id == snapshotID ) 
        {
            overwrite = true;
            break; 
        }
    }


    //add a new snapshot
    if( ! overwrite )
    {
        // Add snapshot to the Dom representation and list of snapshots
        if( rootNode )
        {
            // construct new node for new snapshot
            Element* snapshotElement = NULL;
            ostringstream os;   
            snapshotElement = rootNode->add_child( "snapshot" );
            os.str( "" );
            os << snapshotID;
            snapshotElement->set_attribute( "id", os.str() );
            os.str( "" );
            os << name;
            snapshotElement->set_attribute( "name", os.str() );

            // copy source and group data from scenario
            Scenario* newSnapshot = new Scenario( *scenario, snapshotElement );
            newSnapshot->id       = snapshotID;
            newSnapshot->name     = name;

            // validate and add snapshot to the list of snapshots
            validate();
            snapshots.push_back( newSnapshot );
        }
        else
            return 1; // can not get root node
    }
    else // overwrite the existing snapshot with data from scenario
    {
        Scenario* snapshot = *iter;
        // set the new name
        snapshot->name = name;

        // loop through all sources in the scenario 
        for( int i = 0; i < ( int ) scenario->sourcesVector.size(); ++i )
        {
            // check if source is active in the scenario 
            if( scenario->sourcesVector[ i ].active )
            {
                // if the source was not already active in the old snapshot,
                // then add it to the DOM representation
                if( ! snapshot->sourcesVector[ i ].active  )
                    snapshot->activateSource( i );

                // copy source data from the new snapshot
                 snapshot->sourcesVector[ i ] = scenario->sourcesVector[ i ];
            }
            else // source is not active in scenario
            {
                // if it is active in the old snaphot then deactivate it 
                // and remove it from the DOM representation
                if( snapshot->sourcesVector[ i ].active )
                    snapshot->deactivateSource( i );
            }
        }
        // loop through all groups in the scenario 
        for( int i = 0; i < ( int ) scenario->sourceGroupsVector.size(); ++i )
        {
            // check if source is active in the scenario 
            if( scenario->sourceGroupsVector[ i ].active )
            {
                // if the source was not already active in the old snapshot,
                // then add it to the DOM representation
                if( ! snapshot->sourceGroupsVector[ i ].active  )
                    snapshot->activateGroup( i + 1 );

                // copy group data from the new snapshot
                 snapshot->sourceGroupsVector[ i ] = scenario->sourceGroupsVector[ i ];
            }
            else // group is not active in scenario
            {
                // if it is active in the old snaphot then deactivate it 
                // and remove it from the DOM representation
                if( snapshot->sourceGroupsVector[ i ].active )
                    snapshot->deactivateGroup( i + 1 );
            }
        }
    }
    return 0;
}


int Project::recallSnapshot( int snapshotID )
{
    // in scenario activate all sources and groups which are active in snapshot 
    // and set their data
    // deactivate the other sources and groups

    // get the right snapshot
    Scenario* recall = NULL;
    list< Scenario* >::iterator iter;
    for( iter = snapshots.begin(); iter != snapshots.end(); ++iter)
    {
        if( ( *iter )->id == snapshotID )
        {
            recall = *iter;
            break;
        }
    }
    if( recall == NULL )
        return -1; // snapshot does not exist

    // loop through all sources 
    for( int i = 0; i < maxNoSources; ++i)
    {
        if( recall->sourcesVector[ i ].active ) // source active in recalled snapshot
        {
            // if it wasn't active, then activate source in scenario
            if( ! scenario->sourcesVector[ i ].active )
                scenario->activateSource( i );

            // copy data of this source to scenario
            scenario->sourcesVector[ i ] = recall->sourcesVector[ i ];

        }
        else // source is not active in recalled snapshot
        {
            // if it was active, then deactivate source in scenario
            if( scenario->sourcesVector[ i ].active )
                scenario->deactivateSource( i );
        }
    }
    // loop through all groups 
    for( int i = 0; i < maxNoSources; ++i)
    {
        if( recall->sourceGroupsVector[ i ].active ) // group active in recalled snapshot
        {
            // if it wasn't active, then activate group in scenario
            if( ! scenario->sourceGroupsVector[ i ].active )
                scenario->activateGroup( i + 1 );

            // copy data of this group to scenario
            scenario->sourceGroupsVector[ i ] = recall->sourceGroupsVector[ i ];

        }
        else // group is not active in recalled snapshot
        {
            // if it was active, then deactivate group in scenario
            if( scenario->sourceGroupsVector[ i ].active )
                scenario->deactivateGroup( i + 1 );
        }
    }
    return 0;
}


int Project::deleteSnapshot( int snapshotID )
{ 
    // the scenario may only be deleted by ~Project
    if( snapshotID == 0)
        return 1; // snapshot does not exist

    list< Scenario* >::iterator iter;
    for( iter= snapshots.begin(); iter != snapshots.end(); ++iter)
    {
        if( ( *iter )->id == snapshotID )
        {
            // remove the sources and groups from the dom representation of the snapshot
            Node::NodeList nList = ( ( *iter )->node )->get_children();
            for( Node::NodeList::iterator niter = nList.begin(); niter != nList.end(); ++niter )
                ( *iter )->node->remove_child( ( *niter ) );
        
            // remove snapshot from the dom representation
            rootNode->remove_child( ( *iter )->node );

            // delete snapshot and remove it from the snapshots list 
            delete *iter;
            snapshots.erase( iter );    
            
            if( validate() == 1 )
                return 2; // error validation

            return 0;
        }               
    }
    return 1; // snapshot does not exist
}


int Project::copySnapshot( int fromID, int toID )
{ 
    list< Scenario* >::iterator fromIter;

    // we don't need to check scenario 
    
    for( fromIter = ++( snapshots.begin() ); fromIter != snapshots.end(); ++fromIter ) 
    {
        // if snapshot which should be copied from does exist
        if( ( *fromIter )->id == fromID )
        {
            list< Scenario* >::iterator toIter;
            for( toIter = snapshots.begin(); toIter != snapshots.end(); ++toIter )
            {
                if( ( *toIter )->id == toID  ||  toID == 0 ) 
                    return 2; // the target snapshot already exist or tried to copy to scenario
            }

            // Add snapshot to the Dom representation and list of snapshots
            if( rootNode )
            {
                // construct new node for new snapshot
                Element* newSnapshotElement = NULL;
                ostringstream os;   
                newSnapshotElement = rootNode->add_child( "snapshot" );
                os.str( "" );
                os << toID;
                newSnapshotElement->set_attribute( "id", os.str() );
                os.str( "" );
                os << ( *fromIter )->name;
                newSnapshotElement->set_attribute( "name", os.str() );

                // copy source data from fromID to toID
                Scenario* newSnapshot = new Scenario( *( *fromIter ), newSnapshotElement );
                newSnapshot->id   = toID;
                newSnapshot->name = ( *fromIter )->name;
                snapshots.push_back( newSnapshot );
            }
            return 0;
        }
    }
    return 1; // snapshot that you want to copy from does not exit
}


int Project::renameSnapshot( int snapshotID, string name )
{ 
    list< Scenario* >::iterator iter;
    for( iter = snapshots.begin(); iter != snapshots.end(); ++iter)
    {
        if( ( *iter  )->id == snapshotID )
        {
            ( *iter  )->name = name;
            if( validate() == 1 )
                return 2; // error validating
            else
                return 0;
        }               
    }
    return 1; // snapshot does not exist
}


void Project::readSnapshotsFromDOM( const Node* node, const Glib::ustring& xmlPath )
{
    NodeSet nSet = node->find( xmlPath );
    NodeSet::iterator iter;
    for( iter = nSet.begin(); iter != nSet.end(); ++iter )
    {
        Scenario* t = new Scenario( *iter, maxNoSources ); 
        snapshots.push_back(t) ;        
    }
}


void Project::writeSnapshotsToDOM()
{
    list< Scenario* >::iterator iter;
    for( iter = snapshots.begin(); iter != snapshots.end(); ++iter )
        ( *iter )->writeToDOM();
}

//-----------------------------end of Project---------------------------------//



//--------------------------------Scenario-----------------------------------//

Scenario::Scenario( int maxNoSources ) : maxNoSources( maxNoSources )
{
    Source      source;
    SourceGroup sourceGroup;

    // initialize with max number of sources and groups possible
    for( int i = 0; i < maxNoSources; i++ )
    {
        source.id = i;  
        sourcesVector.push_back( source );

        sourceGroup.id = i + 1;  
        sourceGroupsVector.push_back( sourceGroup );
    }    

    id   = 0;
    node = NULL; 
}


Scenario::Scenario( Node* n, int maxNoSources ): maxNoSources( maxNoSources )
{
    Source      source;
    SourceGroup sourceGroup;

    // initialize with max number of sources possible
    for( int i = 0; i < maxNoSources; i++ )
    {
        source.id = i;  
        sourcesVector.push_back( source );

        sourceGroup.id = i + 1;  
        sourceGroupsVector.push_back( sourceGroup );
    }    

    id   = 0;
    node = n;
    readFromDOM();
}


Scenario::Scenario( const Scenario& other, Node* node )
{
    id           = other.id;
    name         = other.name;
    maxNoSources = other.maxNoSources;
    this->node   = node; 

    Source      source;
    SourceGroup sourceGroup;

    // initialize with max number of sources possible
    //  copy data for active sources and groups
    for( int i = 0; i < maxNoSources; ++i )
    {
        source.id = i;  
        sourcesVector.push_back( source );
        if( other.sourcesVector.at( i ).active )
        {
            this->activateSource( i );
            this->sourcesVector.at( i ) = other.sourcesVector.at( i );
        }
    }

    for( int i = 0; i < maxNoSources; ++i )
    {
        sourceGroup.id = i + 1;  
        sourceGroupsVector.push_back( sourceGroup );
        if( other.sourceGroupsVector.at( i ).active )
        {
            this->activateGroup( i + 1 );
            this->sourceGroupsVector.at( i ) = other.sourceGroupsVector.at( i );
        }
    }    
}


Scenario::~Scenario()
{
    while( ! sourcesVector.empty() )
        sourcesVector.pop_back();

    while( ! sourceGroupsVector.empty() )
        sourceGroupsVector.pop_back();
}


void Scenario::readFromDOM() 
{ 
    if( const Element* nodeElement = dynamic_cast< const Element* >( node ) ) 
    {
        // read scenario data
        const Element::AttributeList& attributes = nodeElement->get_attributes();
        for( Element::AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter )
        {
            const Attribute* attribute = *iter;

            attribName = attribute->get_name();         
            istringstream is( attribute->get_value() );
            if ( attribName == "id" )
                is >> id; 
            if ( attribName == "name" )
                name = attribute->get_value(); 
        }               
            
        // read data of all sources
        Node::NodeList childList = node->get_children();

        for( Node::NodeList::iterator childListIter = childList.begin(); 
             childListIter != childList.end(); ++childListIter )
        {
            if( Element* childElement = dynamic_cast< Element* >( *childListIter ) ) 
            {
                elementName = childElement->get_name();
                if( elementName == "source" )
                {
                    int id = -1;
                    const Element::AttributeList& srcAttribs = childElement->get_attributes();
                    for( Element::AttributeList::const_iterator srcAttrIter = srcAttribs.begin(); 
                        srcAttrIter != srcAttribs.end(); ++srcAttrIter)
                    {
                        const Attribute* attribute = *srcAttrIter;                      
                        attribName = attribute->get_name();             
                        istringstream is( attribute->get_value() );
                        if ( attribName == "id" )
                        {
                            is >> id;
                            //If id is invalid as index into sourcesVector an out_of_range exception is thrown and
                            //terminates parsing. 
                            sourcesVector.at( id ).id = id;
                        }
                        else if ( attribName == "type" )
                            is >> sourcesVector.at( id ).type;
                        else if ( attribName == "name" )
                            sourcesVector.at( id ).name = attribute->get_value(); 
                        else if ( attribName == "posx" )
                            is >> sourcesVector.at( id ).pos[ 0 ];
                        else if ( attribName == "posy" )
                            is >> sourcesVector.at( id ).pos[ 1 ];
                        else if ( attribName == "angle" )
                            is >> sourcesVector.at( id ).angle;
                        else if ( attribName == "groupId" )
                            is >> sourcesVector.at( id ).groupID;
                        else if ( attribName == "colorR" )
                            is >> sourcesVector.at( id ).color[ 0 ];
                        else if ( attribName == "colorG" )
                            is >> sourcesVector.at( id ).color[ 1 ];
                        else if ( attribName == "colorB" )
                            is >> sourcesVector.at( id ).color[ 2 ];
                        else if ( attribName == "invRotation" )
                            is >> sourcesVector.at( id ).invertedRotationDirection;
                        else if ( attribName == "invScaling" )
                            is >> sourcesVector.at( id ).invertedScalingDirection;
                        else if ( attribName == "doppler" )
                            is >> sourcesVector.at( id ).dopplerEffect;
                    }

                    // check for missing attributes ( can only happen with those marked as implied in the DTD )
                    // and ad them to the current element
                    ostringstream os;
                    if( childElement->find( "@groupId" ).empty() )
                    {
                        os << sourcesVector.at( id ).groupID;
                        childElement->set_attribute( "groupId", os.str() );
                    }
                    if( childElement->find( "@colorR" ).empty() )
                    {
                        os << sourcesVector.at( id ).color[ 0 ];
                        childElement->set_attribute( "colorR", os.str() );
                    }
                    if( childElement->find( "@colorG" ).empty() )
                    {
                        os << sourcesVector.at( id ).color[ 1 ];
                        childElement->set_attribute( "colorG", os.str() );
                    }
                    if( childElement->find( "@colorB" ).empty() )
                    {
                        os << sourcesVector.at( id ).color[ 2 ];
                        childElement->set_attribute( "colorB", os.str() );
                    }
                    if( childElement->find( "@invRotation" ).empty() )
                    {
                        os << sourcesVector.at( id ).invertedRotationDirection;
                        childElement->set_attribute( "invRotation", os.str() );
                    }
                    if( childElement->find( "@invScaling" ).empty() )
                    {
                        os << sourcesVector.at( id ).invertedScalingDirection;
                        childElement->set_attribute( "invScaling", os.str() );
                    }
                    if( childElement->find( "@doppler" ).empty() )
                    {
                        os << sourcesVector.at( id ).dopplerEffect;
                        childElement->set_attribute( "doppler", os.str() );
                    }

                    // mark source as active and set its node
                    sourcesVector.at( id ).active = true;
                    sourcesVector.at( id ).node   = childElement;        
                }
                else if( elementName == "group" )
                {
                    int id = -1;
                    const Element::AttributeList& grpAttribs = childElement->get_attributes();
                    for( Element::AttributeList::const_iterator grpAttrIter = grpAttribs.begin(); 
                        grpAttrIter != grpAttribs.end(); ++grpAttrIter)
                    {
                        const Attribute* attribute = *grpAttrIter;                      
                        attribName = attribute->get_name();             
                        istringstream is( attribute->get_value() );
                        if ( attribName == "id" )
                        {
                            is >> id;
                            --id;
                            //If id is invalid as index into sourceGroupsVector
                            //an out_of_range exception is thrown and terminates parsing. 
                            sourceGroupsVector.at( id ).id = id + 1;
                        }
                        else if ( attribName == "posx" )
                            is >> sourceGroupsVector.at( id ).pos[ 0 ];
                        else if ( attribName == "posy" )
                            is >> sourceGroupsVector.at( id ).pos[ 1 ];
                        else if ( attribName == "colorR" )
                            is >> sourceGroupsVector.at( id ).color[ 0 ];
                        else if ( attribName == "colorG" )
                            is >> sourceGroupsVector.at( id ).color[ 1 ];
                        else if ( attribName == "colorB" )
                            is >> sourceGroupsVector.at( id ).color[ 2 ];
                    }

                    // check for missing attributes ( can only happen with those marked as implied in the DTD )
                    // and ad them to the current element
                    ostringstream os;
                    if( childElement->find( "@colorR" ).empty() )
                    {
                        os << sourceGroupsVector.at( id ).color[ 0 ];
                        childElement->set_attribute( "colorR", os.str() );
                    }
                    if( childElement->find( "@colorG" ).empty() )
                    {
                        os << sourceGroupsVector.at( id ).color[ 1 ];
                        childElement->set_attribute( "colorG", os.str() );
                    }
                    if( childElement->find( "@colorB" ).empty() )
                    {
                        os << sourceGroupsVector.at( id ).color[ 2 ];
                        childElement->set_attribute( "colorB", os.str() );
                    }

                    // mark group as active and set its node
                    sourceGroupsVector.at( id ).active = true;
                    sourceGroupsVector.at( id ).node   = childElement;        
                }
            }
        }
    }
}


void Scenario::writeToDOM() 
{
    Element::AttributeList::const_iterator scenarioAttrIter;
    Element::AttributeList::const_iterator srcAttrIter;
    Element::AttributeList::const_iterator grpAttrIter;

    if( const Element* nodeElement = dynamic_cast< const Element* >( node ) ) 
    {   
        // write scenario data
        const Element::AttributeList& attributes = nodeElement->get_attributes();
        for( scenarioAttrIter = attributes.begin(); scenarioAttrIter != attributes.end(); ++scenarioAttrIter )
        {
            Attribute* attribute = *scenarioAttrIter;

            attribName = attribute->get_name();         
            ostringstream os;
            if ( attribName == "id" )
                os << id;
            else if ( attribName == "name" )
                os << name;
                   
            const Glib::ustring out = os.str();
            attribute->set_value( out );
        }       

        // write source data
        for( int i = 0 ; i < ( int ) sourcesVector.size(); i++ )
        {           
            if( sourcesVector[ i ].active )
            {
                if( const Element* sourceElement = dynamic_cast< const Element* >( sourcesVector[ i ].node ) )
                { 
                    const Element::AttributeList& srcAttribs = sourceElement->get_attributes();
                    for( srcAttrIter = srcAttribs.begin(); srcAttrIter != srcAttribs.end(); ++srcAttrIter)
                    {
                        Attribute* srcAttrib = *srcAttrIter;

                        attribName = srcAttrib->get_name();             
                        ostringstream os;
                        if ( attribName == "id" )
                            os << sourcesVector[ i ].id;
                        else if ( attribName == "type" )
                            os << sourcesVector[ i ].type;
                        else if ( attribName == "name" )
                            os << sourcesVector[ i ].name;
                        else if ( attribName == "posx" )
                            os << sourcesVector[ i ].pos[ 0 ];
                        else if ( attribName == "posy" )
                            os << sourcesVector[ i ].pos[ 1 ];
                        else if ( attribName == "angle" )
                            os << sourcesVector[ i ].angle;
                        else if ( attribName == "groupId" )
                            os << sourcesVector[ i ].groupID;
                        else if ( attribName == "colorR" )
                            os << sourcesVector[ i ].color[ 0 ];
                        else if ( attribName == "colorG" )
                            os << sourcesVector[ i ].color[ 1 ];
                        else if ( attribName == "colorB" )
                            os << sourcesVector[ i ].color[ 2 ];
                        else if ( attribName == "invRotation" )
                            os << sourcesVector[ i ].invertedRotationDirection;
                        else if ( attribName == "invScaling" )
                            os << sourcesVector[ i ].invertedScalingDirection;
                        else if ( attribName == "doppler" )
                            os << sourcesVector[ i ].dopplerEffect;
                
                        const Glib::ustring out = os.str();
                        srcAttrib->set_value( out );
                    }
                }
            }
        }
        // write group data
        for( int i = 0 ; i < ( int ) sourceGroupsVector.size(); i++ )
        {           
            if( sourceGroupsVector[ i ].active )
            {
                if( const Element* groupElement = dynamic_cast< const Element* >( sourceGroupsVector[ i ].node ) )
                { 
                    const Element::AttributeList& grpAttribs = groupElement->get_attributes();
                    for( grpAttrIter = grpAttribs.begin(); grpAttrIter != grpAttribs.end(); ++grpAttrIter)
                    {
                        Attribute* grpAttrib = *grpAttrIter;

                        attribName = grpAttrib->get_name();             
                        ostringstream os;
                        if ( attribName == "id" )
                            os << sourceGroupsVector[ i ].id;
                        else if ( attribName == "posx" )
                            os << sourceGroupsVector[ i ].pos[ 0 ];
                        else if ( attribName == "posy" )
                            os << sourceGroupsVector[ i ].pos[ 1 ];
                        else if ( attribName == "colorR" )
                            os << sourceGroupsVector[ i ].color[ 0 ];
                        else if ( attribName == "colorG" )
                            os << sourceGroupsVector[ i ].color[ 1 ];
                        else if ( attribName == "colorB" )
                            os << sourceGroupsVector[ i ].color[ 2 ];
                
                        const Glib::ustring out = os.str();
                        grpAttrib->set_value( out );
                    }
                }
            }
        }
    }
}


void Scenario::activateSource( int id )
{ 
    // activate all the sources 
    if( id == -1 )
    {
        for( int i = 0; i < ( int ) sourcesVector.size(); i++ )
        {
            sourcesVector[ i ].active = true; 
            // if this is the first time this source is activated then
            // add it to the dom representation
            // but only if a parent node does exist
            if( sourcesVector[ i ].node == NULL  &&  node != NULL )
            {
                ostringstream os;
                Element* srcElement = node->add_child( "source" );
                os.str( "" );
                os << sourcesVector[ i ].id;
                srcElement->set_attribute( "id", os.str() );
                    
                os.str( "" );
                os << sourcesVector[ i ].type;
                srcElement->set_attribute( "type", os.str() );

                os.str( "" );
                os << sourcesVector[ i ].name;
                srcElement->set_attribute( "name", os.str() ); 
                    
                os.str( "" );
                os << sourcesVector[ i ].pos[ 0 ];
                srcElement->set_attribute( "posx", os.str() );
                    
                os.str( "" );
                os << sourcesVector[ i ].pos[ 1 ];
                srcElement->set_attribute( "posy", os.str() );
                    
                os.str( "" );
                os << sourcesVector[ i ].angle;
                srcElement->set_attribute( "angle", os.str() );

                os.str( "" );
                os << sourcesVector[ i ].groupID;
                srcElement->set_attribute( "groupId", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].color[ 0 ];
                srcElement->set_attribute( "colorR", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].color[ 1 ];
                srcElement->set_attribute( "colorG", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].color[ 2 ];
                srcElement->set_attribute( "colorB", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].invertedRotationDirection;
                srcElement->set_attribute( "invRotation", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].invertedScalingDirection;
                srcElement->set_attribute( "invScaling", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].dopplerEffect;
                srcElement->set_attribute( "doppler", os.str() );

                sourcesVector[ i ].node = srcElement;
            }
        }
    }
    else // activate just one source
    {
        //mandatory boundschecking, operator[] of std::vector is unchecked!
        if( ( id >= 0 ) && ( id < ( int ) sourcesVector.size() ) )
        {
            sourcesVector[ id ].active = true; 
            // if this is the first time this source is activated then
            // add it to the dom representation
            if( sourcesVector[ id ].node == NULL  &&  node != NULL )
            {
                ostringstream os;
                Element* srcElement = node->add_child( "source" );
                os.str( "" );
                os << sourcesVector[ id ].id;
                srcElement->set_attribute( "id", os.str() );
                    
                os.str( "" );
                os << sourcesVector[ id ].type;
                srcElement->set_attribute( "type", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].name;
                srcElement->set_attribute( "name", os.str() ); 
                    
                os.str( "" );
                os << sourcesVector[ id ].pos[ 0 ];
                srcElement->set_attribute( "posx", os.str() );
                    
                os.str( "" );
                os << sourcesVector[ id ].pos[ 1 ];
                srcElement->set_attribute( "posy", os.str() );
                    
                os.str( "" );
                os << sourcesVector[ id ].angle;
                srcElement->set_attribute( "angle", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].groupID;
                srcElement->set_attribute( "groupId", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].color[ 0 ];
                srcElement->set_attribute( "colorR", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].color[ 1 ];
                srcElement->set_attribute( "colorG", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].color[ 2 ];
                srcElement->set_attribute( "colorB", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].invertedRotationDirection;
                srcElement->set_attribute( "invRotation", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].invertedScalingDirection;
                srcElement->set_attribute( "invScaling", os.str() );

                os.str( "" );
                os << sourcesVector[ id ].dopplerEffect;
                srcElement->set_attribute( "doppler", os.str() );

                sourcesVector[ id ].node = srcElement;
            }
        }
    }
}


void Scenario::deactivateSource( int id )
{ 
    // deactivate all the sources (or just one source)
    if( id == -1 )
    {
        for( int i = 0; i < ( int ) sourcesVector.size(); i++ )
        {
            //deactivate and reset to default values
            Source* temp  = &( sourcesVector[ i ] );
            
            temp->active  = false; 
            //temp->id      = 0; // id should be kept in case of reactivation
            temp->type    = 1;
            temp->name    = "";
            temp->angle   = 0.0;
            temp->groupID = 0;

            temp->pos[ 0 ] = 0.0;
            temp->pos[ 1 ] = 0.0;

            temp->color[ 0 ] = 0;
            temp->color[ 1 ] = 255;
            temp->color[ 2 ] = 0;

            temp->invertedRotationDirection = false;
            temp->invertedScalingDirection  = false;

            temp->dopplerEffect = true;

            if( sourcesVector[ i ].node  &&  node != NULL )
                node->remove_child( sourcesVector[ i ].node );

            sourcesVector[ i ].node = NULL;
        }

    }
    else
    {
        //mandatory boundschecking, operator[] of std::vector is unchecked!
        if( ( id >= 0 ) && ( id < ( int ) sourcesVector.size() ) )
        {
            //deactivate and reset to default values
            Source* temp  = &( sourcesVector[ id ] );
            
            temp->active  = false; 
            //temp->id      = 0; // id should be kept in case of reactivation
            temp->type    = 1;
            temp->name    = "";
            temp->angle   = 0.0;
            temp->groupID = 0;

            temp->pos[ 0 ] = 0.0;
            temp->pos[ 1 ] = 0.0;

            temp->color[ 0 ] = 0;
            temp->color[ 1 ] = 255;
            temp->color[ 2 ] = 0;

            temp->invertedRotationDirection = false;
            temp->invertedScalingDirection  = false;

            temp->dopplerEffect = true;

            if( sourcesVector[ id ].node  && node != NULL )
                node->remove_child( sourcesVector[ id ].node );

            sourcesVector[ id ].node = NULL;
        }
    }
}


void Scenario::activateGroup( int groupID )
{ 
    int id = groupID - 1;
    //mandatory boundschecking, operator[] of std::vector is unchecked!
    if( ( id >= 0 ) && ( id < ( int ) sourceGroupsVector.size() ) )
    {
        sourceGroupsVector[ id ].active = true; 
        // if this is the first time this group is activated then
        // add it to the dom representation
        if( sourceGroupsVector[ id ].node == NULL  &&  node != NULL )
        {
            ostringstream os;
            Element* grpElement = node->add_child( "group" );
            os.str( "" );
            os << sourceGroupsVector[ id ].id;
            grpElement->set_attribute( "id", os.str() );
                
            os.str( "" );
            os << sourceGroupsVector[ id ].pos[ 0 ];
            grpElement->set_attribute( "posx", os.str() );
                
            os.str( "" );
            os << sourceGroupsVector[ id ].pos[ 1 ];
            grpElement->set_attribute( "posy", os.str() );

            os.str( "" );
            os << sourceGroupsVector[ id ].color[ 0 ];
            grpElement->set_attribute( "colorR", os.str() );

            os.str( "" );
            os << sourceGroupsVector[ id ].color[ 1 ];
            grpElement->set_attribute( "colorG", os.str() );

            os.str( "" );
            os << sourceGroupsVector[ id ].color[ 2 ];
            grpElement->set_attribute( "colorB", os.str() );
                
            sourceGroupsVector[ id ].node = grpElement;
        }
    }
}


void Scenario::deactivateGroup( int groupID )
{ 
    int id = groupID - 1;
    //mandatory boundschecking, operator[] of std::vector is unchecked!
    if( ( id >= 0 ) && ( id < ( int ) sourceGroupsVector.size() ) )
    {
        sourceGroupsVector[ id ].active = false; 

        if( sourceGroupsVector[ id ].node  &&  node != NULL )
            node->remove_child( sourceGroupsVector[ id ].node );

        sourceGroupsVector[ id ].node = NULL;
    }
}


string Scenario::show( )
{ 
    ostringstream log;
    log << "Show scenario id="<< id << endl;
 
    for( int i = 0; i < ( int ) sourcesVector.size(); i++ )
    {   
        Source& source = sourcesVector[ i ];
        if( source.active ) 
        {
            log << endl
                << "Source id="                  << source.id
                << " type="                      << source.type  
                << " x="                         << source.pos[ 0 ]
                << " y="                         << source.pos[ 1 ]
                << " angle="                     << source.angle
                << " groupId="                   << source.groupID 
                << " colorR="                    << source.color[ 0 ]
                << " colorG="                    << source.color[ 1 ]
                << " colorB="                    << source.color[ 2 ]
                << " invertedRotationDirection=" << source.invertedRotationDirection
                << " invertedScalingDirection="  << source.invertedScalingDirection
                << " doppler="                   << source.dopplerEffect
                << endl; 
        }
    }
    for( int i = 0; i < ( int ) sourceGroupsVector.size(); i++ )
    {   
        SourceGroup& sourceGroup = sourceGroupsVector[ i ];
        if( sourceGroup.active ) 
        {
            log << endl
                << "Group id="  << sourceGroup.id
                << " x="        << sourceGroup.pos[ 0 ]
                << " y="        << sourceGroup.pos[ 1 ]
                << " colorR="   << sourceGroup.color[ 0 ]
                << " colorG="   << sourceGroup.color[ 1 ]
                << " colorB="   << sourceGroup.color[ 2 ]
                << endl; 
        }
    }
    return log.str();
}

//----------------------------end of Scenario--------------------------------//



//---------------------------------Source------------------------------------//

Source::Source()
{
    // set default values
    node    = NULL;
    id      = -1;
    type    = 1;
    name    = "";    
    active  = false;
    angle   = 0.f;
    groupID = 0;

    // default color is green 
    color[ 0 ] = 0;
    color[ 1 ] = 255;
    color[ 2 ] = 0;

    invertedRotationDirection = false;
    invertedScalingDirection  = false;
    dopplerEffect             = true;
}


Source::~Source()
{
    node = NULL;
}

Source& Source::operator = ( Source const& other )
{
    id      = other.id;
    type    = other.type;
    pos     = other.pos;
    name    = other.name;
    active  = other.active;
    angle   = other.angle;
    groupID = other.groupID;

    color[ 0 ] = other.color[ 0 ];
    color[ 1 ] = other.color[ 1 ];
    color[ 2 ] = other.color[ 2 ];

    invertedRotationDirection = other.invertedRotationDirection;
    invertedScalingDirection  = other.invertedScalingDirection;
    dopplerEffect             = other.dopplerEffect;

    return *this;
}

//-----------------------------end of Source----------------------------------//

//------------------------------SourceGroup-----------------------------------//

SourceGroup::SourceGroup()
{
    // set default values
    node    = NULL;
    id      = -1;
    active  = false;

    // default color is red
    color[ 0 ] = 255;
    color[ 1 ] = 0;
    color[ 2 ] = 0;
}


SourceGroup::~SourceGroup()
{
    node = NULL;
}


SourceGroup& SourceGroup::operator = ( SourceGroup const& other )
{
    id     = other.id;
    pos    = other.pos;
    active = other.active;

    color[ 0 ] = other.color[ 0 ];
    color[ 1 ] = other.color[ 1 ];
    color[ 2 ] = other.color[ 2 ];

    return *this;
}

//---------------------------end of SourceGroup-------------------------------//
