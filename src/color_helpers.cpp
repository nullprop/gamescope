#include "color_helpers.h"

#include <cstdint>
#include <cmath>
#include <algorithm>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/string_cast.hpp>

glm::vec3 xyY_to_XYZ( const glm::vec2 & xy, float Y )
{
    if ( fabsf( xy.y ) < std::numeric_limits<float>::min() )
    {
        return glm::vec3( 0.f );
    }
    else
    {
        return glm::vec3( Y * xy.x / xy.y, Y, ( 1.f - xy.x - xy.y ) * Y / xy.y );
    }
}

glm::vec2 XYZ_to_xy( const glm::vec3 & XYZ )
{
    float sum = ( XYZ.x + XYZ.y + XYZ.z );
    if ( fabsf( sum ) < std::numeric_limits<float>::min() )
    {
        return glm::vec2( 0.f );
    }
    else
    {
        return glm::vec2( XYZ.x / sum, XYZ.y / sum );
    }
}

glm::vec3 xy_to_xyz( const glm::vec2 & xy )
{
    return glm::vec3( xy.x, xy.y, 1.f - xy.x - xy.y );
}

// Convert xy to CIE 1976 u'v'
glm::vec2 xy_to_uv( const glm::vec2 & xy )
{
    float denom = -2.f * xy.x + 12.f * xy.y + 3.f;
    if ( fabsf( denom ) < std::numeric_limits<float>::min() )
    {
        return glm::vec2( 0.f );
    }

    return glm::vec2( 4.f * xy.x / denom, 9.f * xy.y / denom );
}

// Convert CIE 1976 u'v' to xy
glm::vec2 uv_to_xy( const glm::vec2 & uv )
{
    float denom = 6.f * uv.x - 16.f * uv.y + 12.f;
    if ( fabsf( denom ) < std::numeric_limits<float>::min() )
    {
        return glm::vec2( 0.f );
    }

    return glm::vec2( 9.f * uv.x / denom, 4.f * uv.y / denom );
}

glm::mat3 normalised_primary_matrix( const primaries_t & rgbPrimaries, const glm::vec2 & whitePrimary, float whiteLuminance )
{
    glm::mat3 matPrimaries( xy_to_xyz( rgbPrimaries.r ), xy_to_xyz( rgbPrimaries.g ), xy_to_xyz( rgbPrimaries.b ) );
    glm::vec3 whiteXYZ = xyY_to_XYZ( whitePrimary, whiteLuminance );
    glm::mat3 whiteScale = glm::diagonal3x3( glm::inverse( matPrimaries ) * whiteXYZ );
    return matPrimaries * whiteScale;
}

enum EChromaticAdaptationMethod
{
    k_EChromaticAdapatationMethod_XYZ,
    k_EChromaticAdapatationMethod_Bradford,
};

glm::mat3 chromatic_adaptation_matrix( const glm::vec3 & sourceWhite, const glm::vec3 & destWhite, EChromaticAdaptationMethod eMethod )
{
    static const glm::mat3 k_matBradford( 0.8951f,-0.7502f, 0.0389f, 0.2664f,1.7135f,  -0.0685f, -0.1614f,  0.0367f, 1.0296f );
    glm::mat3 matAdaptation = eMethod == k_EChromaticAdapatationMethod_XYZ ? glm::diagonal3x3( glm::vec3(1,1,1) ) : k_matBradford;
    glm::vec3 coneResponseDest = matAdaptation * destWhite;
    glm::vec3 coneResponseSource = matAdaptation * sourceWhite;
    glm::vec3 scale = glm::vec3( coneResponseDest.x / coneResponseSource.x, coneResponseDest.y / coneResponseSource.y, coneResponseDest.z / coneResponseSource.z );
    return glm::inverse( matAdaptation ) * glm::diagonal3x3( scale ) * matAdaptation;
}

displaycolorimetry_t lerp( const displaycolorimetry_t & a, const displaycolorimetry_t & b, float t )
{
    displaycolorimetry_t result;
    primaries_t a_uv, b_uv;
    a_uv.r = xy_to_uv( a.primaries.r );
    a_uv.g = xy_to_uv( a.primaries.g );
    a_uv.b = xy_to_uv( a.primaries.b );

    b_uv.r = xy_to_uv( b.primaries.r );
    b_uv.g = xy_to_uv( b.primaries.g );
    b_uv.b = xy_to_uv( b.primaries.b );

    glm::vec2 a_white = xy_to_uv( a.white );
    glm::vec2 b_white = xy_to_uv( b.white );

    result.primaries.r.x = flerp( a_uv.r.x, b_uv.r.x, t );
    result.primaries.r.y = flerp( a_uv.r.y, b_uv.r.y, t );
    result.primaries.g.x = flerp( a_uv.g.x, b_uv.g.x, t );
    result.primaries.g.y = flerp( a_uv.g.y, b_uv.g.y, t );
    result.primaries.b.x = flerp( a_uv.b.x, b_uv.b.x, t );
    result.primaries.b.y = flerp( a_uv.b.y, b_uv.b.y, t );
    result.white.x = flerp( a_white.x, b_white.x, t );
    result.white.y = flerp( a_white.y, b_white.y, t );

    result.primaries.r = uv_to_xy( result.primaries.r );
    result.primaries.g = uv_to_xy( result.primaries.g );
    result.primaries.b = uv_to_xy( result.primaries.b );
    result.white = uv_to_xy( result.white );

    return result;
}

colormapping_t lerp( const colormapping_t & a, const colormapping_t & b, float t )
{
    colormapping_t result;
    result.blendEnableMinSat = flerp( a.blendEnableMinSat, b.blendEnableMinSat, t );
    result.blendEnableMaxSat = flerp( a.blendEnableMaxSat, b.blendEnableMaxSat, t );
    result.blendAmountMin = flerp( a.blendAmountMin, b.blendAmountMin, t );
    result.blendAmountMax = flerp( a.blendAmountMax, b.blendAmountMax, t );
    return result;
}

glm::vec3 hsv_to_rgb( const glm::vec3 & hsv )
{
    if ( fabsf( hsv.y ) < std::numeric_limits<float>::min() )
    {
       return glm::vec3( hsv.z ) ;
    }

    float flHue = positive_mod( hsv.x, 1.f );
    flHue *= 6.f;

    int i = flHue;        // integer part
	float f = flHue - i;    // fractional part

	float p = hsv.z * ( 1.f - hsv.y );
	float q = hsv.z * ( 1.f - hsv.y * f );
	float t = hsv.z * ( 1.f - hsv.y * ( 1.f - f ) );

	switch(i)
	{
	case 0: return glm::vec3( hsv.z, t, p ); break;
	case 1: return glm::vec3( q, hsv.z, p ); break;
	case 2: return glm::vec3( p, hsv.z, t ); break;
	case 3: return glm::vec3( p, q, hsv.z ); break;
	case 4: return glm::vec3( t, p, hsv.z ); break;
	case 5: return glm::vec3( hsv.z, p, q ); break;
	}

    return glm::vec3( 0 );
}


glm::vec3 rgb_to_hsv( const glm::vec3 & rgb )
{
    float flMax = std::max( std::max( rgb.x, rgb.y ), rgb.z );
    float flMin = std::min( std::min( rgb.x, rgb.y ), rgb.z );
    float flDelta = flMax - flMin;

    glm::vec3 hsv;
    hsv.y = ( fabsf( flMax ) < std::numeric_limits<float>::min() ) ? 0.f : flDelta / flMax;
    hsv.z = flMax;

    if (hsv.y == 0.f)
    {
        hsv.x = -1.0f;
    }
    else
    {
        if ( rgb.x == flMax )
        {
            hsv.x = (rgb.y - rgb.z) / flDelta;
        }
        else if ( rgb.y == flMax )
        {
            hsv.x = 2.f + ( rgb.z - rgb.x ) / flDelta;
        }
        else
        {
            hsv.x = 4.f + ( rgb.x - rgb.y ) / flDelta;
        }

        hsv.x /= 6.f;

        if ( hsv.x < 0.f ) 
        {
            hsv.x += 1.f;
        }
    }

    return hsv;
}

bool BOutOfGamut( const glm::vec3 & color )
{
    return ( color.x<0.f || color.x > 1.f || color.y<0.f || color.y > 1.f || color.z<0.f || color.z > 1.f );
}

glm::vec3 calcEOTFToLinear( const glm::vec3 & input, EOTF eotf, const tonemapping_t & tonemapping )
{
    if ( eotf == EOTF_Gamma22 )
    {
        return glm::pow( input, glm::vec3( 2.2f ) ) * tonemapping.g22_luminance_tolinear;
    }
    else if ( eotf == EOTF_PQ )
    {
        return glm::vec3( pq_to_nits( input.r ),  pq_to_nits( input.g ), pq_to_nits( input.b ) );
    }

    return glm::vec3(0);
}

glm::vec3 calcLinearToEOTF( const glm::vec3 & input, EOTF eotf, const tonemapping_t & tonemapping )
{
    if ( eotf == EOTF_Gamma22 )
    {
        glm::vec3 val = input;
        if ( tonemapping.g22_luminance_fromlinear > 0.f )
        {
            val = glm::clamp( input / tonemapping.g22_luminance_fromlinear, glm::vec3( 0.f ), glm::vec3( 1.f ) );
        }
        return glm::pow( val, glm::vec3( 1.f/2.2f ) );
    }
    else if ( eotf == EOTF_PQ )
    {
        return glm::vec3( nits_to_pq( input.r ), nits_to_pq( input.g ), nits_to_pq( input.b ) );
    }

    return glm::vec3(0);
}

// input is from 0->1
// TODO: use tone-mapping for white, black, contrast ratio
glm::vec3 applyShaper( const glm::vec3 & input, EOTF source, EOTF dest, const tonemapping_t & tonemapping, bool bInvert )
{
    if ( source == dest || !tonemapping.bUseShaper )
    {
        return input;
    }

    if ( bInvert )
    {
        // (once we do tone mapping this gets more complicated
        std::swap( source, dest );
    }

    return calcLinearToEOTF( calcEOTFToLinear( input, source, tonemapping ), dest, tonemapping );
}

void calcColorTransform( uint16_t * pRgbxData1d, int nLutSize1d,
    uint16_t * pRgbxData3d, int nLutEdgeSize3d,
	const displaycolorimetry_t & source, EOTF sourceEOTF,
	const displaycolorimetry_t & dest,  EOTF destEOTF,
    const colormapping_t & mapping, const nightmode_t & nightmode, const tonemapping_t & tonemapping )
{
    glm::mat3 xyz_from_dest = normalised_primary_matrix( dest.primaries, dest.white, 1.f );
    glm::mat3 dest_from_xyz = glm::inverse( xyz_from_dest );
    glm::mat3 xyz_from_source = normalised_primary_matrix( source.primaries, source.white, 1.f );
    glm::mat3 dest_from_source = dest_from_xyz * xyz_from_source; // Absolute colorimetric mapping

    // Generate shaper lut
    // Note: while this is typically a 1D approximation of our end to end transform,
    // it need not be! Conceptually this is just to determine the interpolation properties...
    // The 3d lut should be considered a 'matched' pair where the transform is only complete
    // when applying both.  I.e., you can put ANY transform in here, and it should work.

    if ( pRgbxData1d )
    {
        float flScale = 1.f / ( (float) nLutSize1d - 1.f );

        for ( int nVal=0; nVal<nLutSize1d; ++nVal )
        {
            glm::vec3 sourceColorEOTFEncoded = { nVal * flScale, nVal * flScale, nVal * flScale };
            glm::vec3 shapedSourceColor = applyShaper( sourceColorEOTFEncoded, sourceEOTF, destEOTF, tonemapping, false );
            pRgbxData1d[nVal * 4 + 0] = drm_quantize_lut_value( shapedSourceColor.x );
            pRgbxData1d[nVal * 4 + 1] = drm_quantize_lut_value( shapedSourceColor.y );
            pRgbxData1d[nVal * 4 + 2] = drm_quantize_lut_value( shapedSourceColor.z );
            pRgbxData1d[nVal * 4 + 3] = 0;
        }
    }

    if ( pRgbxData3d )
    {
        float flScale = 1.f / ( (float) nLutEdgeSize3d - 1.f );

        // Precalc night mode scalars
        // amount and saturation are overdetermined but we separate the two as they conceptually represent
        // different quantities, and this preserves forwards algorithmic compatibility
        glm::vec3 nightModeMultHSV( nightmode.hue, clamp01( nightmode.saturation * nightmode.amount ), 1.f );
        glm::vec3 vNightModeMultLinear = glm::pow( hsv_to_rgb( nightModeMultHSV ), glm::vec3( 2.2f ) );

        for ( int nBlue=0; nBlue<nLutEdgeSize3d; ++nBlue )
        {
            for ( int nGreen=0; nGreen<nLutEdgeSize3d; ++nGreen )
            {
                for ( int nRed=0; nRed<nLutEdgeSize3d; ++nRed )
                {
                    glm::vec3 shapedSourceColor = { nRed * flScale, nGreen * flScale, nBlue * flScale };
                    glm::vec3 sourceColorEOTFEncoded = applyShaper( shapedSourceColor, sourceEOTF, destEOTF, tonemapping, true );

                    // Convert to linearized display referred for source colorimetry
                    glm::vec3 sourceColorLinear = calcEOTFToLinear( sourceColorEOTFEncoded, sourceEOTF, tonemapping );

                    // Convert to dest colorimetry (linearized display referred)
                    glm::vec3 destColorLinear = dest_from_source * sourceColorLinear;

                    // Do a naive blending with native gamut based on saturation
                    // ( A very simplified form of gamut mapping )
                    // float colorSaturation = rgb_to_hsv( sourceColor ).y;
                    float colorSaturation = rgb_to_hsv( sourceColorLinear ).y;
                    float amount = cfit( colorSaturation, mapping.blendEnableMinSat, mapping.blendEnableMaxSat, mapping.blendAmountMin, mapping.blendAmountMax );
                    destColorLinear = glm::mix( destColorLinear, sourceColorLinear, amount );

                    // Apply night mode
                    destColorLinear = vNightModeMultLinear * destColorLinear;

                    // Apply dest EOTF
                    glm::vec3 destColorEOTFEncoded = calcLinearToEOTF( destColorLinear, destEOTF, tonemapping );

                    // Write LUT
                    int nLutIndex = nBlue * nLutEdgeSize3d * nLutEdgeSize3d + nGreen * nLutEdgeSize3d + nRed;
                    pRgbxData3d[nLutIndex * 4 + 0] = drm_quantize_lut_value( destColorEOTFEncoded.x );
                    pRgbxData3d[nLutIndex * 4 + 1] = drm_quantize_lut_value( destColorEOTFEncoded.y );
                    pRgbxData3d[nLutIndex * 4 + 2] = drm_quantize_lut_value( destColorEOTFEncoded.z );
                    pRgbxData3d[nLutIndex * 4 + 3] = 0;
                }
            }
        }
    }
}

bool BIsWideGamut( const displaycolorimetry_t & nativeDisplayOutput )
{
    // Use red as a sentinal for a wide-gamut display
    return ( nativeDisplayOutput.primaries.r.x > 0.650f && nativeDisplayOutput.primaries.r.y < 0.320f );
}

void buildSDRColorimetry( displaycolorimetry_t * pColorimetry, colormapping_t *pMapping,
	float flSDRGamutWideness, const displaycolorimetry_t & nativeDisplayOutput  )
{
    if ( BIsWideGamut( nativeDisplayOutput) )
    {
        // 0.0: 709
        // 1.0: Native
        colormapping_t noRemap;
        noRemap.blendEnableMinSat = 0.7f;
        noRemap.blendEnableMaxSat = 1.0f;
        noRemap.blendAmountMin = 0.0f;
        noRemap.blendAmountMax = 0.0f;
        *pMapping = noRemap;
        *pColorimetry = lerp( displaycolorimetry_709, nativeDisplayOutput, flSDRGamutWideness );
    }
    else
    {
        // 0.0: Native
        // 0.5: Generic wide gamut display w/smooth mapping
        // 1.0: Generic wide gamut display w/harsh mapping

        // This is a full blending to the unit cube, starting at 70% max sat
        // Creates a smooth transition from in-gamut to out of gamut
        colormapping_t smoothRemap;
        smoothRemap.blendEnableMinSat = 0.7f;
        smoothRemap.blendEnableMaxSat = 1.0f;
        smoothRemap.blendAmountMin = 0.0f;
        smoothRemap.blendAmountMax = 1.f;

        // Assume linear saturation computation
        // This is a partial (25%) blending to the unit cube
        // Allows some (but not full) clipping
        colormapping_t partialRemap;
        partialRemap.blendEnableMinSat = 0.7f;
        partialRemap.blendEnableMaxSat = 1.0f;
        partialRemap.blendAmountMin = 0.0f;
        partialRemap.blendAmountMax = 0.25;

        if ( flSDRGamutWideness < 0.5f )
        {
            float t = cfit( flSDRGamutWideness, 0.f, 0.5f, 0.0f, 1.0f );
            *pColorimetry = lerp( nativeDisplayOutput, displaycolorimetry_widegamutgeneric, t );
            *pMapping = smoothRemap;
        }
        else
        {
            float t = cfit( flSDRGamutWideness, 0.5f, 1.0f, 0.0f, 1.0f );
            *pColorimetry = displaycolorimetry_widegamutgeneric;
            *pMapping = lerp( smoothRemap, partialRemap, t );
        }
    }
}

void buildPQColorimetry( displaycolorimetry_t * pColorimetry, colormapping_t *pMapping, const displaycolorimetry_t & nativeDisplayOutput )
{
    *pColorimetry = displaycolorimetry_2020;

    if ( BIsWideGamut( nativeDisplayOutput) )
    {
        colormapping_t smoothRemap;
        smoothRemap.blendEnableMinSat = 0.7f;
        smoothRemap.blendEnableMaxSat = 1.0f;
        smoothRemap.blendAmountMin = 0.0f;
        smoothRemap.blendAmountMax = 1.f;
        *pMapping = smoothRemap;
    }
    else
    {
        colormapping_t noRemap;
        noRemap.blendEnableMinSat = 0.7f;
        noRemap.blendEnableMaxSat = 1.0f;
        noRemap.blendAmountMin = 0.0f;
        noRemap.blendAmountMax = 0.0f;
        *pMapping = noRemap;
    }
}

bool approxEqual( const glm::vec3 & a, const glm::vec3 & b, float flTolerance = 1e-5f )
{
    glm::vec3 v = glm::abs(a - b);
    return ( v.x < flTolerance && v.y < flTolerance && v.z < flTolerance );
}

int writeRawLut( const char * outputFilename, uint16_t * pData, size_t nSize )
{
    FILE *file = fopen( outputFilename, "wb" );
    if ( !file )
    {
        return 1;
    }
    for ( size_t i=0; i<nSize; ++i )
    {
        fwrite( pData + i, sizeof( uint16_t ), 1, file );
    }
    fclose( file );
    return 0;
}

int color_tests()
{
#if 0
    {
        // Test normalized primary matrix
        primaries_t primaries = { { 0.602f, 0.355f }, { 0.340f, 0.574f }, { 0.164f, 0.121f } };
        glm::vec2 white = { 0.3070f, 0.3220f };

        glm::mat3x3 rgb_to_xyz = normalised_primary_matrix( primaries, white, 1.f );
        printf("normalised_primary_matrix rgb_to_xyz %s\n", glm::to_string(rgb_to_xyz).c_str() );

        glm::vec3 redXYZ = rgb_to_xyz * glm::vec3(1,0,0);
        glm::vec2 redxy = XYZ_to_xy( redXYZ );
        glm::vec3 whiteXYZ = rgb_to_xyz * glm::vec3(1);
        glm::vec2 whitexy = XYZ_to_xy( whiteXYZ );

        printf("r xy %s == %s \n", glm::to_string(primaries.r).c_str(), glm::to_string(redxy).c_str() );
        printf("w xy %s == %s \n", glm::to_string(white).c_str(), glm::to_string(whitexy).c_str() );
    }
#endif


#if 0
    {
        // chromatic adapatation
        glm::vec3 d50XYZ = glm::vec3(0.96422f, 1.00000f, 0.82521f );
        glm::vec3 d65XYZ = glm::vec3(0.95047f, 1.00000f, 1.08883f );
        printf("d50XYZ %s\n", glm::to_string(d50XYZ).c_str() );
        printf("d65XYZ %s\n", glm::to_string(d65XYZ).c_str() );
        {
            
            glm::mat3x3 d65From50 = chromatic_adaptation_matrix( d50XYZ, d65XYZ, k_EChromaticAdapatationMethod_Bradford );
            printf("bradford d65From50 %s\n", glm::to_string(d65From50).c_str() );
            glm::vec3 d65_2 = d65From50 * d50XYZ;
            printf("bradford d65_2 %s\n", glm::to_string(d65_2).c_str() );
        }
        {
            
            glm::mat3x3 d65From50 = chromatic_adaptation_matrix( d50XYZ, d65XYZ, k_EChromaticAdapatationMethod_XYZ );
            printf("xyzscaling d65From50 %s\n", glm::to_string(d65From50).c_str() );
            glm::vec3 d65_2 = d65From50 * d50XYZ;
            printf("xyzscaling d65_2 %s\n", glm::to_string(d65_2).c_str() );
        }
    }
#endif

#if 0
    {

        int nLut3DSize = 4;
        float flScale = 1.f / ( (float) nLut3DSize - 1.f );
        for (int nBlue = 0; nBlue<nLut3DSize; ++nBlue )
        {
            for (int nGreen = 0; nGreen<nLut3DSize; ++nGreen )
            {
                for (int nRed = 0; nRed<nLut3DSize; ++nRed )
                {
                    glm::vec3 rgb1( nRed * flScale, nGreen * flScale, nBlue * flScale );
                    glm::vec3 hsv = rgb_to_hsv( rgb1 );
                    glm::vec3 rgb2 = hsv_to_rgb( hsv );
                    if ( !approxEqual(rgb1, rgb2 ) )
                    {
                        printf("****");
                    }
                    printf("%s %s %s\n", glm::to_string(rgb1).c_str(), glm::to_string(hsv).c_str(), glm::to_string(rgb2).c_str()  );
                }
            }
        }
    }
#endif

   return 0;
}
