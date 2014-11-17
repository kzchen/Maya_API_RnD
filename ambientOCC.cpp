// C++ lib	
#include <iostream>
#include <math.h>

// basic lib
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>


// attribute lib
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>

// type
#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MIntArray.h>

// Render utility
#include <maya/MRenderUtil.h>
 
#define MAXIMUM 10000
#define PI 3.1415926


// attribute declaration
MObject aMode;
MObject aColor1;
MObject aColor2;
MObject aNormalCamera;
MObject aPointCamera;
MObject aSamples;
MObject aSpread;
MObject aMaxDistance;
MObject aRaySampler;
 
MObject aOutColor;
MTypeId plugID(0x123499);

MStatus status;


 
class myAO : public MPxNode
{
public: 
 
	static void* creator()
	{
		return new myAO;
	}
 
	static MStatus initialize()
	{
		MFnNumericAttribute numericAttrFn;
		MFnEnumAttribute enumAttrFn;

		// Inputs Attributed
		//
		aMode = enumAttrFn.create( "mode", "mod", 0 );
		enumAttrFn.addField( "Normal_Occlusion", 0 );
		enumAttrFn.addField( "Reflective_Occlusion", 1 );
		addAttribute(aMode);

		// bright color
		aColor1 = numericAttrFn.createColor( "darkColor", "c1", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault( 0.0, 0.0, 0.0 );
		addAttribute(aColor1);

		// dark color
		aColor2 = numericAttrFn.createColor( "brightColor", "c2", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault( 1.0, 1.0, 1.0 );
		addAttribute(aColor2);

		// Surface normal in camera space 
		aNormalCamera = numericAttrFn.createPoint( "normalCamera", "n", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setHidden(true);
		addAttribute(aNormalCamera);

		// xyz location of geometry in camera space 
		aPointCamera = numericAttrFn.createPoint( "pointCamera", "p", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setHidden(true);
		addAttribute(aPointCamera);
		
		
		// 	Pointer to the raytracer from the renderer
		aRaySampler = numericAttrFn.createAddr( "raySampler", "rsp");
		numericAttrFn.setHidden(true);
		addAttribute(aRaySampler);
		
		
		// shooting samples
		aSamples = numericAttrFn.create("samples", "s", MFnNumericData::kInt, 0.0, &status);
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault(4);
		numericAttrFn.setMin(1);
		numericAttrFn.setSoftMax(256);
		addAttribute(aSamples);
		
 
		// angle for cone distribution 
		aSpread = numericAttrFn.create( "spread", "spr", MFnNumericData::kFloat, 0.0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault(0.5);
		numericAttrFn.setMin(0.0);
		numericAttrFn.setMax(1.0);
		addAttribute(aSpread);
		
		// max distance
		aMaxDistance = numericAttrFn.create( "maxDistance", "mds", MFnNumericData::kFloat, 0.0, &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setDefault(0.0);
		numericAttrFn.setMin(0.0);
		addAttribute(aMaxDistance);

 
		// outputs Attributes
		//
		aOutColor = numericAttrFn.createColor( "outColor", "oc", &status );
		CHECK_MSTATUS(status);
		numericAttrFn.setWritable(false);
		addAttribute(aOutColor);

		// affects attributes

		attributeAffects( aColor1, aOutColor );
		attributeAffects( aColor2, aOutColor );
		attributeAffects( aMode, aOutColor );
		attributeAffects( aNormalCamera, aOutColor );
		attributeAffects( aPointCamera, aOutColor );
		attributeAffects( aSamples, aOutColor );
		attributeAffects( aSpread, aOutColor );
		attributeAffects( aMaxDistance, aOutColor );
 
		return MS::kSuccess;
	}

	// get a normalize verctor
	MFloatVector randomDirection()
	{
		double x = ( (double)( random() % MAXIMUM )/ MAXIMUM ) * 2.0 - 1.0;
		double y = ( (double)( random() % MAXIMUM )/ MAXIMUM ) * 2.0 - 1.0;
		double z = ( (double)( random() % MAXIMUM )/ MAXIMUM ) * 2.0 - 1.0;
		MFloatVector outVec( x, y, z );
		outVec.normalize();

		return outVec;
	}
 
	MFloatVector getReflectVector( MVector& incidentVec, MVector& axisNormal )
	{
		// The incident normal is ( camera_pos-->PointAtFace )
		// projection on vector( u-->v )
		// proj_u = v * ( proj_u.length() / v.length() ) = proj_u = v * (cos(angle) * u.length()) / v.length()
		// ref_v = d + proj_u = (u + proj_u) + proj_u

		MVector proj_u = (-1) * axisNormal * ( incidentVec * axisNormal )/ pow(axisNormal.length(), 2) ;
		MVector vd = incidentVec + proj_u;
		MVector reflVec = vd + proj_u;
		reflVec.normalize();
		return (MFloatVector)reflVec;
	}

 
	virtual MStatus compute(const MPlug& plug, MDataBlock& dataBlock)
	{
		if ( plug != aOutColor )
			return MS::kSuccess;
		
		int mode = dataBlock.inputValue(aMode).asInt();
		MFloatVector normalCamera = dataBlock.inputValue(aNormalCamera).asFloatVector();
		MFloatVector pointCamera = dataBlock.inputValue(aPointCamera).asFloatVector();
		void* raySampler = dataBlock.inputValue(aRaySampler).asAddr();

		// actual samples are the square of the input samples
		int samples = pow( (float)dataBlock.inputValue(aSamples).asInt(), 2 );

		float angle = dataBlock.inputValue(aSpread).asFloat() * 180.0;
 
		MFloatVectorArray origins(samples);
		MFloatVectorArray directions(samples);
		MFloatVectorArray intersections(samples);
		MIntArray didHits(samples);


		// To decide the center axis that samples spread toward in a cone direction
		MFloatVector axisVector( 0.0, 0.0, 0.0 );
		if ( mode == 0 )
			axisVector = normalCamera;

		if ( mode == 1 )
		{
			// get the reflective vector by the incident vector ( camera-->origins[i])
			//
			axisVector = getReflectVector( (MVector)pointCamera, (MVector)normalCamera );
			axisVector.normalize();
		}

		
		// for each samples
		//
		for (int i = 0; i < samples; i++)
		{
			// hit point on object from camera
			origins[i] = pointCamera;

			MFloatVector randomVector = randomDirection();

			// corss vector
			MFloatVector tangentVector = (randomVector ^ axisVector).normal();
 
			// random angle, degree to radiance
			double randomAngle = (double)( angle * ( (random() % MAXIMUM)/ MAXIMUM ) * (PI/ 180.0) ) ;

			// to get a project vector
			MFloatVector nrmNormal = axisVector * cos(randomAngle);
			MFloatVector nrmTangent = tangentVector * sin(randomAngle);
			MFloatVector nrmBi = nrmNormal + nrmTangent;
 
			directions[i] = nrmBi;
		}
 
		MRenderUtil::raytraceFirstGeometryIntersections(origins,
														directions,
														NULL,
														raySampler,
														intersections,
														didHits);


		float maxDist = dataBlock.inputValue(aMaxDistance).asFloat();

		int hits = 0;
 
		for (int i = 0; i < samples; i++)
		{
			if (didHits[i] == 1)
			{
				if ( maxDist > 0.0 )
				{
					// test if the distance(intersection<-->origin) larger than the max distance
					MFloatVector vec = intersections[i] - origins[i];
					if ( vec.length() > maxDist )
						continue;
				}

				hits = hits + 1;
			}
		}


		// ***** OUTPUT *****
		// decide OUTPUT color
		
		float hit_ratio = 1.0 - ( (float)hits/ samples );

		MFloatVector darkColor = dataBlock.inputValue(aColor1).asFloatVector();
		MFloatVector brightColor = dataBlock.inputValue(aColor2).asFloatVector();
		
		// outcolor = color1 + ( color2 - color1 ) * ratio
		
		MFloatVector resultColot = darkColor + (brightColor - darkColor) * hit_ratio;

		dataBlock.outputValue(aOutColor).set(resultColot);
 
		return MS::kSuccess;
	}
 
};
 
// plugin registration/ deregistration
 
MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, "KZi", "1.0", "Any");
 
	MString classification("utility/color");
 
	plugin.registerNode("AO_kz",
						plugID,
						myAO::creator,
						myAO::initialize,
						MPxNode::kDependNode,
						&classification);
 
	return MS::kSuccess;
}
 
MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);
	plugin.deregisterNode(plugID);
 
	return MS::kSuccess;
}
