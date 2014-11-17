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
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMessageAttribute.h>

// type
#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MPlugArray.h>
#include <maya/MMatrix.h>

// Matrix operation
#include <maya/MTransformationMatrix.h>

// Mesh operation
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MItMeshVertex.h>

// Dag node operation
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>

// Data operation
#include <maya/MDataHandle.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionMask.h>
#include <maya/MFnTransform.h>


using namespace std;


// attribute declaration
MObject aEnable;

MObject aTargetObj;
MObject aLightMsg;
MObject aCamMtx4;
MObject aRefPointMtx4;

MObject aOutValue;
MObject aOutVector;

MTypeId ID(0x184708); 
MStatus status;

class placeHighlight : public MPxNode
{
public:

	static void* creator()
	{
		return new placeHighlight;
	}

	static MStatus initialize()
	{
		MFnEnumAttribute enumAttrFn;
		MFnNumericAttribute numericAttrFn;
		MFnGenericAttribute genericAttrFn;
		MFnMatrixAttribute matrixAttrFn;
		MFnMessageAttribute messageAttrFn;

	
			// enable node
		aEnable = numericAttrFn.create( "enbale", "e", MFnNumericData::kBoolean, 0, &status );
		numericAttrFn.setDefault(0);
		numericAttrFn.setKeyable(true);
		addAttribute(aEnable);

	
		// input attributes
		//
		// target object
		aTargetObj = genericAttrFn.create( "targetObject", "to", &status );
		genericAttrFn.addAccept(MFnData::kMesh);
		genericAttrFn.setStorable(false);
		addAttribute( aTargetObj );
		CHECK_MSTATUS( status );

		// Camera W-Matrix input
		aCamMtx4 = matrixAttrFn.create( "cameraWorldMatrix", "cmx", MFnMatrixAttribute::kDouble, &status );
		CHECK_MSTATUS( status );
		//matrixAttrFn.setHidden(true);
		addAttribute( aCamMtx4 );

		// Reference Point W-Matrix input
		aRefPointMtx4 = matrixAttrFn.create( "pointWorldMatrix", "pmx", MFnMatrixAttribute::kDouble, &status );
		CHECK_MSTATUS( status );
		//matrixAttrFn.setHidden(true);
		addAttribute( aRefPointMtx4 );

		// light message receive
		aLightMsg = messageAttrFn.create( "lightMessage", "lt", &status);
		CHECK_MSTATUS( status );
		addAttribute( aLightMsg );


		// ouput attribute
		//
		// out value for test 
		aOutValue = numericAttrFn.create( "outValue", "o", MFnNumericData::kInt, 0, &status );
		CHECK_MSTATUS( status );
		numericAttrFn.setReadable(false);
		addAttribute( aOutValue );


		// Attribute Affects
		attributeAffects( aEnable, aOutValue );
		attributeAffects( aTargetObj, aOutValue );
		attributeAffects( aCamMtx4, aOutValue );
		attributeAffects( aRefPointMtx4, aOutValue );
		attributeAffects( aLightMsg, aOutValue );

		return MS::kSuccess;
	}


	// test if the specified plug(Attr) is connected
	bool isPlugConnect( MObject& inAttr )
	{
		MObject thisNode = thisMObject();		// Get this actual plugin node's MObject
		MPlug plugAttr( thisNode, inAttr );		// Get the attribute on this node

		return plugAttr.isConnected();
	}


	// get dag path from a connected plug/ attribute
	MDagPath getConnectNodeDagPath( MObject& inAttr )
	{
		// First, to get the input plug/ Attr in thisNode()
		// Next, get the input actual node 
		// Last, get the source node's dag path by using dagNodeFn 

		MObject thisNode = thisMObject();	// Get this actual plugin node's MObject

		MPlug plugAttr( thisNode, inAttr );	// Get our attribute on our node
		MDagPath dPathSrcNode;

		if (plugAttr.isConnected())
		{
			// get dagPath
			//
			MPlugArray plugArr;
			plugAttr.connectedTo( plugArr, true, false, &status ) ;	// Now list connections on our attr
			MPlug srcPlug = plugArr[0] ;		// Get the plug that is connected into our input attr.

			MObject oSrcObj = srcPlug.node(&status) ;	// Get the actual node that is connected.
			MFnDagNode dagNodeFn( oSrcObj, &status ) ;	// Put that into a Dag Node Function Set...
			dagNodeFn.getPath( dPathSrcNode ) ;		// Now get the dagPath of this node.

			// print out the dag path as string
			cout << "path: " << dPathSrcNode.fullPathName().asChar() << endl;

			return dPathSrcNode;
		}
	}

	
	// get tanslation from input Matrix Attribute
	MVector getTransFromMatrixAttr( MDataBlock& dataBlock, MObject& attrMtx, MSpace::Space space )
	{
		// get W-Matrix
		MMatrix inMtx4 = dataBlock.inputValue(attrMtx).asMatrix();
		MTransformationMatrix inXform(inMtx4);

		// get translation
		MVector inPos( inXform.getTranslation(space) );
		return inPos;
	}

	// get the reflective normal by incident normal and axis normal
	MVector getReflectVector( MVector& incidentVec, MVector& axisNormal )
	{
		// The incident normal is ( camera_pos-->PointAtFace )
		// projection on vector( u-->v )
		// proj_u = v * ( proj_u.length() / v.length() ) = proj_u = v * (cos(angle) * u.length()) / v.length()
		// ref_v = d + proj_u = (u + proj_u) + proj_u

		MVector proj_u = (-1) * axisNormal * ( incidentVec * axisNormal )/ pow(axisNormal.length(), 2) ;
		MVector vd = incidentVec + proj_u;
		MVector reflVec = vd + proj_u;
		reflVec.normalize();
		return reflVec;
	}

	// create a W-space xForm with a pivot and upVector( y-axis )
	MTransformationMatrix getCustomXform( MPoint& pivot, MVector& upVector )
	{
		MVector x_axis( 1.0, 1.0, -1*(upVector.x + upVector.y)/ upVector.z );		// choose any vector( 1,1,Z ) perpencular to upector(y-axis)
		MVector y_axis = upVector;
		MVector z_axis( x_axis ^ y_axis );		// cross production

		x_axis.normalize();
		y_axis.normalize();
		z_axis.normalize();

		// Z-axis should be mul -1.0 to follow the right-hand coordinate rule
		// switch Y-axis and Z-axis because the light node is Z-up
		double tmp[4][4] = { x_axis.x, x_axis.y, x_axis.z, 0,
							 -1*z_axis.x, -1*z_axis.y, -1*z_axis.z, 0,
							 y_axis.x, y_axis.y, y_axis.z, 0,
							 pivot.x, pivot.y, pivot.z, 1 };

		MMatrix mtx4(tmp);
		MTransformationMatrix xForm(mtx4);

		return xForm;
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

			// check if the specified attributes are connected
			// in Not, stop compute()
			// in order to avoid crash when disconnect input attributes on the fly 
		if ( isPlugConnect(aTargetObj) == false || 
			 isPlugConnect(aCamMtx4) == false ||
			 isPlugConnect(aRefPointMtx4) == false ||
			 isPlugConnect(aLightMsg) == false )
		{
			return MS::kSuccess;
		}


		// execution when output attr needs to be updated
		if ( plug != aOutValue )
		{
			return MS::kSuccess;
		}

		// check input type
		if ( !(dataBlock.inputValue(aTargetObj).type() == MFnData::kMesh) )
		{
			return MS::kSuccess;
		}

		// get position from w-matrix
		//
		MVector pos_cam = getTransFromMatrixAttr( dataBlock, aCamMtx4, MSpace::kWorld );
		MVector pos_refPt = getTransFromMatrixAttr( dataBlock, aRefPointMtx4, MSpace::kWorld );


		// get the closet posint and the surface normal at this point
		//
		// To get the proper world space transformation data, e.g. getClosetPointAndNormal()
		// you MUST use MFnMEsh set with an MDagPath object.
		// so, it's INCORRECT to use MObject targetObj = dataBlock.inputValue(aTargetObj).asMeshTransformed();
		MDagPath dPath_targetObj = getConnectNodeDagPath(aTargetObj);
		MFnMesh meshFn(dPath_targetObj);

		MPoint toThisPoint(pos_refPt);
		MPoint clostPoint( 0.0, 0.0, 0.0 );
		MVector ptAtFaceNormal( 0.0, 0.0, 0.0 );
		int faceID = 0;

		// get the WORLD-SPACE Closet point and Normal
		meshFn.getClosestPointAndNormal( toThisPoint, clostPoint, ptAtFaceNormal, MSpace::kWorld, &faceID );

		// get reflective Vector
		//
		MVector incidentVec = (MVector)clostPoint - pos_cam;
		ptAtFaceNormal.normalize();
		incidentVec.normalize();
		MVector reflVec = getReflectVector( incidentVec, ptAtFaceNormal );

		// get custom xForm
		MTransformationMatrix ptXform = getCustomXform( clostPoint, reflVec );

		// get the xForm from the specified Light
		MDagPath dPathLight = getConnectNodeDagPath(aLightMsg);
		MFnTransform xFormFn( dPathLight );
			
		// get translate vector
		// translate it back to keep the original distance from Light to the closet point 
		MVector oPos_light = xFormFn.getTranslation(MSpace::kWorld);
		double offset = clostPoint.distanceTo( MPoint(oPos_light) );
		MVector offsetVec = MVector( 0.0, 0.0, 1.0 ) * offset;

		// set the Light W-XForm
		xFormFn.set( ptXform );
		xFormFn.translateBy( offsetVec, MSpace::kObject );

		cout << "=================================================================================== "<< endl;
		cout << "clostPoint: " << clostPoint.x << "  " << clostPoint.y << "  " << clostPoint.z << endl;
		cout << "reflVec: " << reflVec.x << "  " << reflVec.y << "  " << reflVec.z << endl;
		cout << "ptAtFaceNormal: " << ptAtFaceNormal.x << "  " << ptAtFaceNormal.y << "  " << ptAtFaceNormal.z << endl;
		cout << "faceID: " << faceID << endl;

		dataBlock.outputValue(aOutValue).set( 1 );

		return MS::kSuccess;
	}
};



MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin pluginFn( obj, "KZ", "2012", "Any");

	pluginFn.registerNode( "placeHighlight",
							ID,
							placeHighlight::creator,
							placeHighlight::initialize,
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
