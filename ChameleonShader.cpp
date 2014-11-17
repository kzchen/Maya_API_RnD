// baisc lib
#include <iostream>
#include <algorithm>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>

// Attribute lib
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnGenericAttribute.h>

// variable type
#include <maya/MFloatVector.h>
#include <maya/MDataHandle.h>

// Nurbs Fn
#include <maya/MFnNurbsSurface.h>

#include <maya/MFnMesh.h>
#include <maya/MRenderUtil.h>

using namespace std;

// Plugin ID
MTypeId pluginID(0x124575);

// Attribute declaration
MObject aEnable;
MObject aEnableRender;
MObject aColor;
MObject aIsSpuersampling;
MObject aFilterSize;
MObject aOffsetSample;
MObject aUVCoord;
MObject aUVFilterSize;
MObject aPointWorld;
MObject aShape;
MObject aTargetShape;
MObject aOutColor;
MObject aFSize;
MStatus status;

class grabTexture : public MPxNode
{
public:
	
	virtual void postConstructor()
	{
		// setMPSafe indicates that this shader can be used for multiprocessor
		// rendering.
        setMPSafe( true );
	}

	static void* creator()
	{
		return new grabTexture();
	}

	static MStatus initialize()
	{
		// set up uder-defined attribute
		MFnNumericAttribute numericAttrFn;
		MFnGenericAttribute genericAttrFn;

		// enable thie node
		aEnable = numericAttrFn.create( "enbale", "e", MFnNumericData::kBoolean, 0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault(0);
		numericAttrFn.setKeyable(true);
		addAttribute(aEnable);

		// enable when rendering
		aEnableRender = numericAttrFn.create( "enbaleAsRendering", "er", MFnNumericData::kBoolean, 0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault(0);
		numericAttrFn.setKeyable(true);
		addAttribute(aEnableRender);


		// Inputs
		//
		// input color
		aColor = numericAttrFn.createColor( "Color", "c", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault( 0.0, 0.5, 0.0);
		addAttribute(aColor);

		// enable supersampling
		aIsSpuersampling = numericAttrFn.create( "supersampling", "ssp", MFnNumericData::kBoolean, 0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault(1);
		numericAttrFn.setKeyable(true);
		addAttribute(aIsSpuersampling);

		// filter size
	/*	aFSize = numericAttrFn.create( "filterSize", "fs", MFnNumericData::k3Float, 0.0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setMin( 0.0, 0.0, 0.0 );
		numericAttrFn.setMax( 10.0, 10.0, 10.0 );
		numericAttrFn.setDefault( 1.0, 1.0, 1.0 );
		addAttribute(aFSize);
*/
		// filter size
		aFilterSize = numericAttrFn.create( "filterSizes", "fls", MFnNumericData::k2Int, 0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setMin( 0, 0 );
		numericAttrFn.setMax( 10, 10 );
		numericAttrFn.setDefault( 0, 0 );
		addAttribute(aFilterSize);

		// sample offset
		aOffsetSample = numericAttrFn.create( "sampleOffset", "ao", MFnNumericData::kFloat, 0.0 , &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setSoftMin( 0.0f );
		numericAttrFn.setSoftMax( 0.1f );
		numericAttrFn.setDefault( 0.01f);
		addAttribute(aOffsetSample);

		// UV coordinate
		MObject uCoord = numericAttrFn.create( "uCoord", "u", MFnNumericData::kDouble );
		MObject vCoord = numericAttrFn.create( "vCoord", "v", MFnNumericData::kDouble );
		aUVCoord = numericAttrFn.create( "uvCoord", "uv", uCoord, vCoord, MObject::kNullObj, &status);
		CHECK_MSTATUS(status);
		numericAttrFn.setHidden(true);
		numericAttrFn.setRenderSource(true);
		addAttribute(aUVCoord);

		// UV filter size
		MObject sUVFilterSizeX = numericAttrFn.create( "uvFilterSizeX", "ufx", MFnNumericData::kFloat );
		MObject sUVFilterSizeY = numericAttrFn.create( "uvFilterSizeY", "ufy", MFnNumericData::kFloat );
		aUVFilterSize = numericAttrFn.create( "uvFilterSize", "uf", sUVFilterSizeX, sUVFilterSizeY, MObject::kNullObj, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setHidden(true);
		addAttribute(aUVFilterSize);

		// World-space position
		aPointWorld = numericAttrFn.createPoint( "pointWorld", "pw", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setHidden(true);
		addAttribute(aPointWorld);

		// input Shape as the source
		aShape = genericAttrFn.create( "sourceShape", "s", &status );
		//genericAttrFn.addDataAccept( MFnData::kNurbsSurface );
		genericAttrFn.addAccept( MFnData::kNurbsSurface );
		genericAttrFn.addAccept( MFnData::kMesh );
		CHECK_MSTATUS(status);
		addAttribute(aShape);

		// target shape
		aTargetShape = genericAttrFn.create( "targetShape", "ts", &status );
		//genericAttrFn.addDataAccept( MFnData::kNurbsSurface );
		genericAttrFn.addAccept( MFnData::kNurbsSurface );
		genericAttrFn.addAccept( MFnData::kMesh );
		CHECK_MSTATUS(status);
		addAttribute(aTargetShape);


		// output
		//
		// output color
		aOutColor = numericAttrFn.createColor( "outColor", "oc", &status);
		CHECK_MSTATUS(status);
		numericAttrFn.setWritable(false);
		addAttribute(aOutColor);


		// affect Attribute
		//attributeAffects( aFSize, aOutColor );
		attributeAffects( aEnable, aOutColor );
		attributeAffects( aEnableRender, aOutColor );
		attributeAffects( aColor, aOutColor );
		attributeAffects( aIsSpuersampling, aOutColor );
		attributeAffects( aFilterSize, aOutColor );
		attributeAffects( aOffsetSample, aOutColor );
		attributeAffects( aUVCoord, aOutColor );
		attributeAffects( aUVFilterSize, aOutColor );
		attributeAffects( aPointWorld, aOutColor );
		attributeAffects( aShape, aOutColor );
		attributeAffects( aTargetShape, aOutColor );

		attributeAffects( aUVCoord, aUVCoord );
		return MS::kSuccess;
	}
	
	
	// Function declaration
	//

	// test if input attribute/ plug is connected
	bool isPlugConnect( MObject& inAttr )
	{
		MObject thisNode = thisMObject();	// Get this actual plugin node's MObject
		MPlug plugAttr( thisNode, inAttr );	// Get the attribute on this node

		return plugAttr.isConnected();
	}


	//
	void getOverlapUVbyNurbs( MObject& nurbsShape, MFloatVector& pointWorld, float uv[] )
	{
		MFnNurbsSurface nurbsSurfaceFn(nurbsShape);

		double rawU, rawV;		// Returns UV "spans" instead of UV coord, so it needs to normalize  
		nurbsSurfaceFn.closestPoint( pointWorld, &rawU, &rawV, false, 1e-6, MSpace::kWorld );

		// normalize UV
		//
		double startU, endU, startV, endV;
		nurbsSurfaceFn.getKnotDomain( startU, endU, startV, endV );

		// output nomalized UV
		uv[0] = (rawU - startU) / (endU - startU);
		uv[1] = (rawV - startV) / (endV - startV);
	}

	//
	void getOverlapUVbyMesh( MObject& meshShape, MFloatVector& pointWorld, float uv[] )
	{
		MFnMesh meshFn(meshShape);

		MPoint closetPoint;
		meshFn.getClosestPoint( MPoint(pointWorld), closetPoint, MSpace::kWorld, NULL );

		float2 uvOut;
		meshFn.getUVAtPoint( closetPoint, uvOut, MSpace::kWorld, NULL, NULL );
		
		// output UV
		uv[0] = uvOut[0];
		uv[1] = uvOut[1];
	}

	//
	MFloatVector doSupersampling( MDataBlock& dataBlock, MObject& aUVCoord, MObject& aColor, float uv[], int x_width, int y_width, float offsetUV )
	{
		MFloatVector sumColor( 0.0, 0.0, 0.0 );
		MFloatVector outColor( 0.0, 0.0, 0.0 );
		offsetUV = offsetUV * 0.1;

		// original UV
		float oUV[] = { uv[0], uv[1] };                                                            
		
		for( int x=0; x<(2*x_width + 1); x++ )
		{
				// offest UV
			float newUV[] = { 0.0, 0.0 };

			if ( x % 2 < 1 )	// if(x == even)
			{
				newUV[0] = oUV[0] - offsetUV * (x/2);
			}
			else
			{
				newUV[0] = oUV[0] + offsetUV * ((x+1)/ 2);
			}

			for ( int y=0; y<(2*y_width + 1); y++ )
			{
				if ( x % 2 < 1 )	// if(y == even)
				{
					newUV[1] = oUV[1] - offsetUV * (y/ 2);
				}
				else
				{
					newUV[1] = oUV[1] + offsetUV * ((y+1)/ 2);
				}

				dataBlock.outputValue(aUVCoord).set( newUV[0], newUV[1] );
				sumColor = sumColor + dataBlock.inputValue(aColor).asFloatVector();
			}
		}
				
		int f_size = (2*x_width + 1) * (2*y_width + 1);
		return outColor = sumColor/ f_size;
	}

	// Main entry
	//
	virtual MStatus compute( const MPlug& plug, MDataBlock& dataBlock )
	{
		// enable this node or not
		if ( dataBlock.inputValue(aEnable).asBool() == false )
			return MS::kSuccess;

		// execution when rendering
		if ( dataBlock.inputValue(aEnableRender).asBool() == true )
			if ( MRenderUtil::mayaRenderState() == MRenderUtil::kNotRendering )
				//MGlobal::displayInfo( "not rendering");
				return MS::kSuccess;

		// Execution when the specified output attributes need to be updated
		if ( !( plug == aOutColor || plug == aUVCoord ) )
			return MS::kSuccess;

		// Check if the aShape is connected 
		if ( isPlugConnect(aShape) != true )
			return MS::kSuccess;


		// From each pixel, sample required info, like point world, UV, .etc.
		//

		// 01. get W-space point
		MDataHandle PointWorldHandle = dataBlock.inputValue( aPointWorld );
		MFloatVector pointWorld = PointWorldHandle.asFloatVector();
		float uv[2];

			// input shape is NURBS Surface
			//
		if ( dataBlock.inputValue( aShape ).type() == MFnData::kNurbsSurface )
		{
			// 02. get each UV for the overlaped area 
			//
			MObject nurbsShape = dataBlock.inputValue(aShape).asNurbsSurface();
			getOverlapUVbyNurbs( nurbsShape, pointWorld, uv );
		}

		// input shape is Mesh
		else if ( dataBlock.inputValue( aShape ).type() == MFnData::kMesh )
		{
			// 02. get each UV for the overlaped area 
			//
			MObject meshShape = dataBlock.inputValue(aShape).asMeshTransformed();	//W-space mesh
			getOverlapUVbyMesh( meshShape, pointWorld, uv );
		}


		// if inpute object is neither Mesh nor Nurbs
		else
		{
			return MS::kSuccess;
		}

		MFloatVector resultColor( 0.0, 0.0, 0.0 );
		// run super sampling function
		if ( dataBlock.inputValue(aIsSpuersampling).asBool() == true )
		{
			float offsetUV = dataBlock.inputValue(aOffsetSample).asFloat();
			int2& filterSize = dataBlock.inputValue(aFilterSize).asInt2();
			int x_width = filterSize[0];
			int y_width = filterSize[1];
			//cout << "super!" << endl;
			resultColor = doSupersampling( dataBlock, aUVCoord, aColor, uv, x_width, y_width, offsetUV );
		}
		else
		{
			// 1. u, v coordinate
			// 2. get get color by uv
			dataBlock.outputValue(aUVCoord).set( uv[0], uv[1] );
			resultColor = dataBlock.inputValue(aColor).asFloatVector();
		}

		dataBlock.outputValue(aOutColor).set(resultColor);

		return MS::kSuccess;
	}
};


MStatus initializePlugin( MObject object )
{
	MFnPlugin pluginFn( object, "KZ", "1.0", "Any" );
	MString classification("shader/surface");
	pluginFn.registerNode( "Grab",
							pluginID,
							grabTexture::creator, 
							grabTexture::initialize,
							MPxNode::kDependNode,
							&classification); 
	return MS::kSuccess;
}

MStatus uninitializePlugin( MObject object )
{
	MFnPlugin pluginFn(object);
	pluginFn.deregisterNode(pluginID);
	return MS::kSuccess;
}
