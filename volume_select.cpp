	// C++ lib	
#include <iostream>
#include <math.h>
#include <vector>

	// basic lib
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

	// attribute lib
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnTypedAttribute.h>
//#include <maya/MFn

	// type
#include <maya/MFloatVector.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MPlugArray.h>

	// Mesh operation
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MItMeshVertex.h>

	// Dag operation
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>

	// data operation
#include <maya/MDataHandle.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionMask.h>
#include <maya/MFnComponentListData.h>


using namespace std;


	// attribute declaration
MObject aEnable;
MObject aFixPanel;
MObject aKeepSel;
MObject aComponentType;
MObject aClosedObj;

MObject aOutValue;
MObject aOutCompList;
MObject aOutMesh;

MObject aSourceObj;
MObject aVolumeObj;

MTypeId ID(0x144788); 
MStatus status;
MSelectionList addSelComponentList;

bool flag = 0;		// a switch for the keep slect 

class volumeSelect : public MPxNode
{
public:

	static void* creator()
	{
		return new volumeSelect;
	}

	static MStatus initialize()
	{
		MFnEnumAttribute enumAttrFn;
		MFnNumericAttribute numericAttrFn;
		MFnGenericAttribute genericAttrFn;
		MFnTypedAttribute typeAttrFn;

		// enable node
		aEnable = numericAttrFn.create( "enbale", "e", MFnNumericData::kBoolean, 0, &status );
		numericAttrFn.setDefault(0);
		numericAttrFn.setKeyable(true);
		addAttribute(aEnable);

		aFixPanel = numericAttrFn.create( "fixPanel", "fpn", MFnNumericData::kBoolean, 0, &status);
		numericAttrFn.setDefault(1);
		numericAttrFn.setConnectable(true);
		addAttribute(aFixPanel);

		aKeepSel = numericAttrFn.create( "keepSelection", "ks", MFnNumericData::kBoolean, 0, &status);
		numericAttrFn.setDefault(0);
		addAttribute(aKeepSel);

		// component type
		aComponentType = enumAttrFn.create( "componetType", "ct", 0, &status );
		CHECK_MSTATUS( status );
		enumAttrFn.setDefault(0);
		enumAttrFn.addField( "vertex", 0 );
		enumAttrFn.addField( "edge", 1 );
		enumAttrFn.addField( "face", 2 );
		enumAttrFn.addField( "uv", 3 );
		addAttribute( aComponentType );
		
		// source object
		aSourceObj = genericAttrFn.create( "sourceObject", "so", &status );
		genericAttrFn.addAccept(MFnData::kMesh);
		genericAttrFn.setStorable(false);
		addAttribute( aSourceObj );
		CHECK_MSTATUS( status );

		// volume object
		aVolumeObj = genericAttrFn.create( "volumeObject", "vo", &status );
		genericAttrFn.addAccept(MFnData::kMesh);
		genericAttrFn.setArray(true);
		addAttribute( aVolumeObj );
		CHECK_MSTATUS( status );

				
		aClosedObj = numericAttrFn.create( "closedVolumeObject", "cvo", MFnNumericData::kBoolean, 1, &status );
		numericAttrFn.setDefault(1);
		addAttribute( aClosedObj );


		// ouput attribute
		//
		aOutValue = numericAttrFn.create( "outValue", "ov", MFnNumericData::kInt,  0, &status );
		CHECK_MSTATUS( status );
		numericAttrFn.setReadable(false);
		addAttribute( aOutValue );

		aOutCompList = typeAttrFn.create( "outComponent", "ocp", MFnComponentListData::kComponentList, &status );
		CHECK_MSTATUS( status );
		addAttribute(aOutCompList);

		aOutMesh = typeAttrFn.create( "outMesh", "om", MFnData::kMesh, &status );
		CHECK_MSTATUS( status );
		addAttribute(aOutMesh);


		// Attribute Affects for aOutValue
		attributeAffects( aEnable, aOutValue );
		attributeAffects( aFixPanel, aOutValue );
		attributeAffects( aComponentType, aOutValue );
		attributeAffects( aKeepSel, aOutValue );
		attributeAffects( aClosedObj, aOutValue );
		attributeAffects( aSourceObj, aOutValue );
		attributeAffects( aVolumeObj, aOutValue );

		//Attribute Affects for aOutCompList
		attributeAffects( aEnable, aOutCompList);
		attributeAffects( aFixPanel, aOutCompList );
		attributeAffects( aComponentType, aOutCompList );
		attributeAffects( aKeepSel, aOutCompList );
		attributeAffects( aClosedObj, aOutCompList );
		attributeAffects( aSourceObj, aOutCompList );
		attributeAffects( aVolumeObj, aOutCompList );

		//Attribute Affects for aOutMesh
		attributeAffects( aEnable, aOutMesh);
		attributeAffects( aFixPanel, aOutMesh );
		attributeAffects( aComponentType, aOutMesh );
		attributeAffects( aKeepSel, aOutMesh );
		attributeAffects( aClosedObj, aOutMesh );
		attributeAffects( aSourceObj, aOutMesh );
		attributeAffects( aVolumeObj, aOutMesh );


		return MS::kSuccess;
	}

	// test if the specified plug(Attr) is connected
	bool isPlugConnect( MObject& inAttr )
	{
		MObject thisNode = thisMObject();	// Get this actual plugin node's MObject
		MPlug plugAttr( thisNode, inAttr );	// Get the attribute on this node

		unsigned int numTotal = plugAttr.numElements();
		unsigned int numConn = plugAttr.numConnectedElements();

		if ( numTotal > 0 )
		{
			if ( numConn == 0 )
				return false;
			else
				return true;
		}

		return plugAttr.isConnected();
	}


	// check if the index element of spceified Attr/ Plug is connected 
	bool isPlugConnect( MObject& inAttr, unsigned idx )
	{
		MObject thisNode = thisMObject();	// Get this actual plugin node's MObject
		MPlug plugAttr( thisNode, inAttr );	// Get the attribute on this node

		if ( idx > plugAttr.numElements()-1 )  
			return false;

		MPlug subPlugAttr = plugAttr[idx];

		return subPlugAttr.isConnected();
	}


	// test if inpute mesh is closed
	bool isClosedMesh( MObject& inMesh )
	{
		MFnMesh meshFn(inMesh);
		int countFaces = meshFn.numPolygons();

			// for each face, check if it's a boundry face
		for( int i=0; i < countFaces; i++ )
		{
			if( meshFn.onBoundary(i) )
				return false;	// clsoed object
		}
		
		return true;	// open object
	}


	//
	MSelectionList convertMIntArrToMselList( MIntArray& intArr, MDagPath& dPath, MFn::Type kCompType )
	{
		MFnSingleIndexedComponent singleIndexCompFn;
			// create MObjet componenet by sigleIndexComponent
		MObject components = singleIndexCompFn.create(kCompType);
		singleIndexCompFn.addElements( intArr );

		MSelectionList compSelList;
		compSelList.add( dPath, components );
		return compSelList;
	}


	//	get DagPath from connected node
	//
	MDagPath getConnectNodeDagPath( MObject& inObject )
	{
		// First, to get the input plug/ Attr in thisNode()
		// Next, get the input actual node 
		// Last, get the source node's dag path by using dagNodeFn 

		MObject thisNode = thisMObject();	// Get this actual plugin node's MObject...that's us!

		MPlug plugAttr( thisNode, inObject );	// Get our attribute on our node
		MDagPath dPathSrcObj;

		if ( plugAttr.isConnected() )
		{
			// get dagPath
			//
			MPlugArray plugArr;
			plugAttr.connectedTo( plugArr, true, false, &status ) ;	// Now list connections on our attr
			MPlug srcPlug = plugArr[0] ;		// Get the plug that is connected into our aSurface attr.

			MObject oSrcObj = srcPlug.node( &status ) ;	// Get the actual node that is connected.
			MFnDagNode dagNodeFn( oSrcObj, &status ) ;	// Put that into a Dag Node Function Set...
			dagNodeFn.getPath( dPathSrcObj ) ;		// Now get the dagPath of this node.

				// print out the dag path as string
			//cout << "path: " << dPathSrcObj.fullPathName().asChar() << endl;

			return dPathSrcObj;
		}
	}

	// ouput the MselectList of VERTEX Component  
	MSelectionList getVtxSelList( MDagPath& inDagPath, const int inCompArray[], int inCountArray )
	{
			// vtx process
			//
		MSelectionList vtxSelList;
			// Create a MFnSingleIndexedComponent object of type kMeshVertComponent.
		MFnSingleIndexedComponent singleIndexCompFn;
		MObject components = singleIndexCompFn.create(MFn::kMeshVertComponent);
		MIntArray elementArray( inCompArray, inCountArray);

			// Add the element indices
		singleIndexCompFn.addElements(elementArray);

			// Add the element to the selection list.
		vtxSelList.add( inDagPath, components );

		return vtxSelList;
	}


	// convert input MSelectionList to the MSelectionlist w/ specified component
	// e.g from vtx MSelectionList to edge MSelectionList
	MSelectionList convertVtxSListToCompSList( MSelectionList& selList, int compType )
	{
		// Two step for conversion
		// 01. convert MSelecitionList to MItSelecitionList
		// 02. run MItMeshVertex

		// if switch to vertex, do nothing
		if (compType == 0)
			return selList;

		MObject vtxComp;
		MDagPath dPathMesh;
		MItSelectionList vertexCompIt( selList, MFn::kMeshVertComponent );
		
		// one-time for loop
		for ( ; !vertexCompIt.isDone(); vertexCompIt.next() )
		{
			vertexCompIt.getDagPath( dPathMesh ,vtxComp );
		}

		MItMeshVertex vertexIt( dPathMesh, vtxComp );
		MSelectionList resultSelList;

		// iterate mesh vertec to get tje connected component
		for( ; !vertexIt.isDone(); vertexIt.next() )
		{
			MSelectionList tmpSelList;
			MIntArray compIdArray;

			// if switch to edge, conver vertex to edge
			if( compType == 1 )
			{
				vertexIt.getConnectedEdges(compIdArray);
				tmpSelList = convertMIntArrToMselList( compIdArray, dPathMesh, MFn::kMeshEdgeComponent );
			}

			// if switch to face, conver vertex to face
			if( compType == 2 )
			{
				vertexIt.getConnectedFaces(compIdArray);
				tmpSelList = convertMIntArrToMselList( compIdArray, dPathMesh, MFn::kMeshPolygonComponent );
			}

			// merge new edge indice into current selection list 
			// if new item has exist, not merge it.
			resultSelList.merge(tmpSelList, MSelectionList::kMergeNormal);
		}

		return resultSelList;
	}


	// extract the component from a MSelectionList
	MObject getCompListFromSList( MSelectionList& sList )
	{
		MDagPath dPath;
		MObject compList;
		sList.getDagPath( 0, dPath, compList );

		MStringArray strArr;
		MGlobal::getFunctionSetList( compList, strArr );
		MGlobal::displayInfo( MString("Array: ") + strArr[0] + strArr[1] );

		return compList;
	}


	
	// ==========================================================================================================
	// ==========================================================================================================
	virtual MStatus compute(const MPlug& plug, MDataBlock& dataBlock)
	{

		// enable this node or not
		if ( dataBlock.inputValue(aEnable).asBool() == false )
		{
			return MS::kSuccess;
		}

		// check if the inpute attribute is connected
		// in Not, stop compute()
		// in order to avoid crash when disconnect input attributes on the fly 
		
		//cout << "isPlugConnect: " << isPlugConnect(aVolumeObj) << endl;
		
		if ( isPlugConnect(aSourceObj) == false || isPlugConnect(aVolumeObj) == false )
		{
			return MS::kSuccess;
		}

		// execution when output attr needs to be updated
		if ( plug == aOutValue || plug == aOutMesh || plug == aOutCompList )
		{
			// test if input source object is a valid type
			if ( dataBlock.inputValue(aSourceObj).type() != MFnData::kMesh )
			{
				MGlobal::displayInfo( MString("No Object Input!") ); 
				return MS::kSuccess;
			}

			MObject sourceObj = dataBlock.inputValue(aSourceObj).asMeshTransformed();

			MArrayDataHandle arrayHandle = dataBlock.inputValue(aVolumeObj);
			arrayHandle.jumpToArrayElement(0);

			MSelectionList sList;		// add the vertice every ligal loop 
			for ( int idx=0; idx < arrayHandle.elementCount(); idx++, arrayHandle.next() )
			{

				// first, check if the sub-plug is un-connected
				if ( isPlugConnect( aVolumeObj, idx ) == false  )
				{
					cout << "No Data " << idx << endl;
					continue;
				}

				// second, check if the input object is mesh
				if ( arrayHandle.inputValue().type() != MFnData::kMesh )
				{
					return MS::kSuccess;
					MGlobal::displayError( "input voulme objects is not mesh" );
				}

				// input volume object as Wrold mesh
				MObject volumeObj = arrayHandle.inputValue().asMeshTransformed();

				MFnMesh sourceMeshFn;
				MFnMesh volumeMeshFn;

				// third, test if the input obj is compatible with meshFn
				if ( volumeMeshFn.hasObj(sourceObj) && volumeMeshFn.hasObj(volumeObj) )
				{
					volumeMeshFn.setObject(volumeObj);

					// check if object is closed
					if ( isClosedMesh(volumeObj) == false )
					{
						if ( dataBlock.inputValue(aClosedObj).asBool() == true )
						{
							//MGlobal::displayInfo( MString("The volume object is not closed!") );
							continue;
						}
					}

					sourceMeshFn.setObject( sourceObj );
					int numVtx = sourceMeshFn.numVertices();

					vector<int> tmpCompArray;	// an temporary int array to store component index

					// do hit test
					// to check if each source's component is inside
					//
					for ( int i=0; i < numVtx; i++ )
					{
						// get each vertex of source object
						MPoint srcVtx;
						sourceMeshFn.getPoint( i, srcVtx, MSpace::kWorld );

						// Test how much hit is for each vertex 
						// declare parameters for allIntersection()
						MFloatPoint raySource;
						raySource.setCast(srcVtx);

						MFloatVector rayDirection(0, 0, 1);
						MFloatPointArray hitPoints;
						MIntArray hitFaces;

						bool hit = volumeMeshFn.allIntersections( raySource,
																	rayDirection,
																	NULL,
																	NULL,
																	false,
																	MSpace::kWorld,
																	99999,
																	false,
																	NULL,
																	true,
																	hitPoints,
																	NULL,
																	&hitFaces,
																	NULL,
																	NULL,
																	NULL,
																	1e-6 );

						if (hit)
						{
							int isInside = hitFaces.length() % 2;
							// cout << "isInside: " << isInside << endl;

							// if the mod is odd, it's inside
							if ( isInside > 0 )
							{
								tmpCompArray.push_back(i);
							}
						}
					}

					// declare a dynamic array to recieve All elements from tmpCompArray
					int* compArray = new int[tmpCompArray.size()];

					// copy array data from tmpCompArray --> compArray
					memcpy( &compArray[0], &tmpCompArray[0], sizeof( int ) * tmpCompArray.size() );


					// the below processes are to collect component data, and then select them in viewport
					//
					// first, get dagPath from the source object 
					MDagPath dPathSrcObj = getConnectNodeDagPath(aSourceObj);


					// second, get the selection list storing components by feeding comopnet array  
					MSelectionList vtxSelList = getVtxSelList( dPathSrcObj, compArray, tmpCompArray.size() );
					sList.merge(vtxSelList);

					delete [] compArray;
				}
			}
			// end of loop


			// if so, actively select these component
			int compType = dataBlock.inputValue(aComponentType).asInt();
			MSelectionList currCompSelList;

			if( dataBlock.inputValue(aKeepSel).asBool() == true )
			{
				// clear if last-time is not keep selection
				if (flag==0)
				{
					addSelComponentList.clear();
					flag = 1; 
				}
				else
				{
					addSelComponentList.merge(sList);	// merge the accumulative components
					currCompSelList = convertVtxSListToCompSList( addSelComponentList, compType );
					MGlobal::setActiveSelectionList( currCompSelList, MGlobal::kReplaceList );
					flag = 1;
				}
			}
			else
			{
				addSelComponentList.clear();	// celar all components
				addSelComponentList.merge(sList);
				currCompSelList = convertVtxSListToCompSList( addSelComponentList, compType );
				MGlobal::setActiveSelectionList( currCompSelList, MGlobal::kReplaceList );
				flag = 0;
			}

			// keep this node selecting 
			if ( dataBlock.inputValue(aFixPanel).asBool() == true )
				MGlobal::select( thisMObject(), MGlobal::kAddToList );


			// **** OUTPUT ATTRIBUTE ****
			MObject currCompList = getCompListFromSList( currCompSelList );
			
			MFnComponentListData compListDataFn;
			MObject currCompListData = compListDataFn.create();		// pointer to the component data
			compListDataFn.add( currCompList );

			dataBlock.outputValue(aOutCompList).set( currCompListData );
			
			// 
			MFnMeshData outMeshDataFn;
			MObject outObj = outMeshDataFn.create();
			MFnMesh outMeshFn( outObj );
			outMeshFn.copy( sourceObj, outObj );

			dataBlock.outputValue(aOutMesh).set( outObj );
		}		
		// end of if ( plug == aOutValue || plug == aOutMesh )

		return MS::kSuccess;
	}
};



MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin pluginFn( obj, "KZ", "2012", "Any");

	pluginFn.registerNode( "volumeSelect",
							ID,
							volumeSelect::creator,
							volumeSelect::initialize,
							MPxNode::kDependNode );
	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin pluginFn( obj );
	pluginFn.deregisterNode( ID );

	return status;
}
