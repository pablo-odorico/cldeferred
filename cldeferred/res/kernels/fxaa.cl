//
// OpenCL adaptation of NVIDIA FXAA 3.11 by TIMOTHY LOTTES (PC Version)
//

float luma(float4 sample)
{
    return dot(sample.xyz, (float3)(0.299f, 0.587f, 0.114f));
}

#define fxaaQualityEdgeThreshold     0.166f
#define fxaaQualityEdgeThresholdMin  0.0833f
#define fxaaQualitySubpix            0.75f

kernel void fxaa(
    read_only  image2d_t input,
    write_only image2d_t output
) {
    const int2 size= get_image_dim(input);
    const float2 rcpSize= (float2)(1.0f/size.x, 1.0f/size.y);

    const int2 ipos= (int2)(get_global_id(0), get_global_id(1));
    const float2 pos= (float2)((ipos.x + 0.5f) * rcpSize.x, (ipos.y + 0.5f) * rcpSize.y);

    if(ipos.x >= size.x || ipos.y >= size.y)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
#define sample(p)          read_imagef(input, sampler, (float2)(p))
#define sampleOffset(xx,yy)  sample(pos + (float2)((xx) * rcpSize.x, (yy) * rcpSize.y))

    float4 sampleM = sample(pos);
    float lumaM = luma(sampleM);
    float lumaS = luma(sampleOffset( 0, 1));
    float lumaE = luma(sampleOffset( 1, 0));
    float lumaN = luma(sampleOffset( 0,-1));
    float lumaW = luma(sampleOffset(-1, 0));

    float maxSM = max(lumaS, lumaM);
    float minSM = min(lumaS, lumaM);
    float maxESM = max(lumaE, maxSM);
    float minESM = min(lumaE, minSM);
    float maxWN = max(lumaN, lumaW);
    float minWN = min(lumaN, lumaW);
    float rangeMax = max(maxWN, maxESM);
    float rangeMin = min(minWN, minESM);
    float rangeMaxScaled = rangeMax * fxaaQualityEdgeThreshold;
    float range = rangeMax - rangeMin;
    float rangeMaxClamped = max(fxaaQualityEdgeThresholdMin, rangeMaxScaled);

    if(range < rangeMaxClamped) {
        write_imagef(output, ipos, sampleM);
        //write_imagef(output, ipos, (float4)(0));
        return;
    }

    float lumaNW = luma(sampleOffset(-1,-1));
    float lumaSE = luma(sampleOffset( 1, 1));
    float lumaNE = luma(sampleOffset( 1,-1));
    float lumaSW = luma(sampleOffset(-1, 1));

    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float subpixRcpRange = 1.0f/range;
    float subpixNSWE = lumaNS + lumaWE;
    float edgeHorz1 = (-2.0f * lumaM) + lumaNS;
    float edgeVert1 = (-2.0f * lumaM) + lumaWE;

    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float edgeHorz2 = (-2.0f * lumaE) + lumaNESE;
    float edgeVert2 = (-2.0f * lumaN) + lumaNWNE;

    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;
    float edgeHorz4 = (fabs(edgeHorz1) * 2.0f) + fabs(edgeHorz2);
    float edgeVert4 = (fabs(edgeVert1) * 2.0f) + fabs(edgeVert2);
    float edgeHorz3 = (-2.0f * lumaW) + lumaNWSW;
    float edgeVert3 = (-2.0f * lumaS) + lumaSWSE;
    float edgeHorz = fabs(edgeHorz3) + edgeHorz4;
    float edgeVert = fabs(edgeVert3) + edgeVert4;

    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float lengthSign = rcpSize.x;
    bool horzSpan = edgeHorz >= edgeVert;
    float subpixA = subpixNSWE * 2.0f + subpixNWSWNESE;

    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    if(horzSpan) lengthSign = rcpSize.y;
    float subpixB = (subpixA * (1.0f/12.0f)) - lumaM;

    float gradientN = lumaN - lumaM;
    float gradientS = lumaS - lumaM;
    float lumaNN = lumaN + lumaM;
    float lumaSS = lumaS + lumaM;
    bool pairN = fabs(gradientN) >= fabs(gradientS);
    float gradient = max(fabs(gradientN), fabs(gradientS));
    if(pairN) lengthSign = -lengthSign;
    float subpixC = clamp(fabs(subpixB) * subpixRcpRange, 0.0f, 1.0f);

    float2 posB;
    posB.x = pos.x;
    posB.y = pos.y;
    float2 offNP;
    offNP.x = (!horzSpan) ? 0.0f : rcpSize.x;
    offNP.y = ( horzSpan) ? 0.0f : rcpSize.y;
    if(!horzSpan) posB.x += lengthSign * 0.5f;
    if( horzSpan) posB.y += lengthSign * 0.5f;

    #define FXAA_QUALITY__PS 5
    #define FXAA_QUALITY__P0 1.0f
    #define FXAA_QUALITY__P1 1.5f
    #define FXAA_QUALITY__P2 2.0f
    #define FXAA_QUALITY__P3 4.0f
    #define FXAA_QUALITY__P4 12.0f

    float2 posN;
    posN.x = posB.x - offNP.x * FXAA_QUALITY__P0;
    posN.y = posB.y - offNP.y * FXAA_QUALITY__P0;
    float2 posP;
    posP.x = posB.x + offNP.x * FXAA_QUALITY__P0;
    posP.y = posB.y + offNP.y * FXAA_QUALITY__P0;
    float subpixD = ((-2.0f)*subpixC) + 3.0f;
    float lumaEndN = luma(sample(posN));
    float subpixE = subpixC * subpixC;
    float lumaEndP = luma(sample(posP));

    if(!pairN) lumaNN = lumaSS;
    float gradientScaled = gradient * 1.0f/4.0f;
    float lumaMM = lumaM - lumaNN * 0.5f;
    float subpixF = subpixD * subpixE;
    bool lumaMLTZero = lumaMM < 0.0f;

    lumaEndN -= lumaNN * 0.5f;
    lumaEndP -= lumaNN * 0.5f;
    bool doneN = fabs(lumaEndN) >= gradientScaled;
    bool doneP = fabs(lumaEndP) >= gradientScaled;
    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P1;
    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P1;
    bool doneNP = (!doneN) || (!doneP);
    if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P1;
    if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P1;

   if(doneNP) {
        if(!doneN) lumaEndN = luma(sample(posN.xy));
        if(!doneP) lumaEndP = luma(sample(posP.xy));
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
        doneN = fabs(lumaEndN) >= gradientScaled;
        doneP = fabs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P2;
        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P2;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P2;
        if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P2;

        #if (FXAA_QUALITY__PS > 3)
        if(doneNP) {
            if(!doneN) lumaEndN = luma(sample(posN.xy));
            if(!doneP) lumaEndP = luma(sample(posP.xy));
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
            doneN = fabs(lumaEndN) >= gradientScaled;
            doneP = fabs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P3;
            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P3;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P3;
            if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P3;

            #if (FXAA_QUALITY__PS > 4)
            if(doneNP) {
                if(!doneN) lumaEndN = luma(sample(posN.xy));
                if(!doneP) lumaEndP = luma(sample(posP.xy));
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                doneN = fabs(lumaEndN) >= gradientScaled;
                doneP = fabs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P4;
                if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P4;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P4;
                if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P4;

                #if (FXAA_QUALITY__PS > 5)
                if(doneNP) {
                    if(!doneN) lumaEndN = luma(sample(posN.xy));
                    if(!doneP) lumaEndP = luma(sample(posP.xy));
                    if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                    if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                    doneN = fabs(lumaEndN) >= gradientScaled;
                    doneP = fabs(lumaEndP) >= gradientScaled;
                    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P5;
                    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P5;
                    doneNP = (!doneN) || (!doneP);
                    if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P5;
                    if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P5;

                    #if (FXAA_QUALITY__PS > 6)
                    if(doneNP) {
                        if(!doneN) lumaEndN = luma(sample(posN.xy));
                        if(!doneP) lumaEndP = luma(sample(posP.xy));
                        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                        doneN = fabs(lumaEndN) >= gradientScaled;
                        doneP = fabs(lumaEndP) >= gradientScaled;
                        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P6;
                        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P6;
                        doneNP = (!doneN) || (!doneP);
                        if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P6;
                        if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P6;

                        #if (FXAA_QUALITY__PS > 7)
                        if(doneNP) {
                            if(!doneN) lumaEndN = luma(sample(posN.xy));
                            if(!doneP) lumaEndP = luma(sample(posP.xy));
                            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                            doneN = fabs(lumaEndN) >= gradientScaled;
                            doneP = fabs(lumaEndP) >= gradientScaled;
                            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P7;
                            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P7;
                            doneNP = (!doneN) || (!doneP);
                            if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P7;
                            if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P7;

    #if (FXAA_QUALITY__PS > 8)
    if(doneNP) {
        if(!doneN) lumaEndN = luma(sample(posN.xy));
        if(!doneP) lumaEndP = luma(sample(posP.xy));
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
        doneN = fabs(lumaEndN) >= gradientScaled;
        doneP = fabs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P8;
        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P8;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P8;
        if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P8;

        #if (FXAA_QUALITY__PS > 9)
        if(doneNP) {
            if(!doneN) lumaEndN = luma(sample(posN.xy));
            if(!doneP) lumaEndP = luma(sample(posP.xy));
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
            doneN = fabs(lumaEndN) >= gradientScaled;
            doneP = fabs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P9;
            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P9;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P9;
            if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P9;

            #if (FXAA_QUALITY__PS > 10)
            if(doneNP) {
                if(!doneN) lumaEndN = luma(sample(posN.xy));
                if(!doneP) lumaEndP = luma(sample(posP.xy));
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                doneN = fabs(lumaEndN) >= gradientScaled;
                doneP = fabs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P10;
                if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P10;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P10;
                if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P10;

                #if (FXAA_QUALITY__PS > 11)
                if(doneNP) {
                    if(!doneN) lumaEndN = luma(sample(posN.xy));
                    if(!doneP) lumaEndP = luma(sample(posP.xy));
                    if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                    if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                    doneN = fabs(lumaEndN) >= gradientScaled;
                    doneP = fabs(lumaEndP) >= gradientScaled;
                    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P11;
                    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P11;
                    doneNP = (!doneN) || (!doneP);
                    if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P11;
                    if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P11;

                    #if (FXAA_QUALITY__PS > 12)
                    if(doneNP) {
                        if(!doneN) lumaEndN = luma(sample(posN.xy));
                        if(!doneP) lumaEndP = luma(sample(posP.xy));
                        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5f;
                        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5f;
                        doneN = fabs(lumaEndN) >= gradientScaled;
                        doneP = fabs(lumaEndP) >= gradientScaled;
                        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P12;
                        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P12;
                        doneNP = (!doneN) || (!doneP);
                        if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P12;
                        if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P12;

                    }
                    #endif

                }
                #endif

            }
            #endif

        }
        #endif

    }
    #endif

                        }
                        #endif

                    }
                    #endif

                }
                #endif

            }
            #endif

        }
        #endif

    }

    float dstN = pos.x - posN.x;
    float dstP = posP.x - pos.x;
    if(!horzSpan) dstN = pos.y - posN.y;
    if(!horzSpan) dstP = posP.y - pos.y;

    bool goodSpanN = (lumaEndN < 0.0f) != lumaMLTZero;
    float spanLength = (dstP + dstN);
    bool goodSpanP = (lumaEndP < 0.0f) != lumaMLTZero;
    float spanLengthRcp = 1.0f/spanLength;

    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float subpixG = subpixF * subpixF;
    float pixelOffset = (dst * (-spanLengthRcp)) + 0.5f;
    float subpixH = subpixG * fxaaQualitySubpix;

    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0f;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);

    float2 outPos= pos;
    if(!horzSpan) outPos.x += pixelOffsetSubpix * lengthSign;
    if( horzSpan) outPos.y += pixelOffsetSubpix * lengthSign;

    write_imagef(output, ipos, sample(outPos));
}
