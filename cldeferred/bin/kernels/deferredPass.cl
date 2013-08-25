
__kernel void deferredPass(
    __read_only image2d_t gbDiffuseSpec,
    __read_only image2d_t gbNormals,
    __read_only image2d_t gbDepth,
    __write_only image2d_t output)
{
    // Get global position
    const int x= get_global_id(0);
    const int y= get_global_id(1);

    // Get output size
    const int width= get_image_width(output);
    const int height= get_image_height(output);

    if(x>=width || y>=height)
        return;

    const sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    float4 diffuseSpec= read_imagef(gbDiffuseSpec, sampler, (int2)(x,y));

    float3 normal= read_imagef(gbNormals, sampler, (int2)(x,y)).xyz;
    normal.z= sqrt(1.0f - normal.x*normal.x - normal.y*normal.y);

    float depth= read_imagef(gbDepth, sampler, (int2)(x,y)).x;

    float4 color= diffuseSpec;

    write_imagef(output, (int2)(x,y), color);
}


    /*
    static int first= 0;
    if(first == 10) {
        float* d= new float[gBuffer.width() * gBuffer.height()];
        if(!d)
            qDebug() << "Alloc error";
        memset(d, 0, sizeof(d));

        size_t origin[3] = {0, 0, 0};
        size_t region[3] = {gBuffer.width(), gBuffer.height(), 1};
        error= clEnqueueReadImage(clQueue(), gbDepth, CL_TRUE, origin, region, 0, 0, d, 0, NULL, NULL);
        if(checkCLError(error, "read image"))
            return;

        QFile file("depth.txt");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        for(int y=0; y<gBuffer.height(); y++) {
            for(int x=0; x<gBuffer.width(); x++) {
                const float z= d[x + y * gBuffer.width()];
                if(!z) continue;

                const float ndcX= (2.0f * x)/gBuffer.width() - 1.0f;
                const float ndcY= (2.0f * y)/gBuffer.height() - 1.0f;
                const float ndcZ= 2.0f * z - 1.0f;

                const float clipW= projMatrix(2,3) / (ndcZ - projMatrix(2,2)/projMatrix(3,2));
                const float clipX= ndcX * clipW;
                const float clipY= ndcY * clipW;
                const float clipZ= ndcZ * clipW;
                QVector4D clipPos(clipX, clipY, clipZ, clipW);

                float eyeZ = projMatrix(2,3) / ((projMatrix(3,2) * ndcZ) - projMatrix(2,2));
                const QVector3D eyeDirection(ndcX, ndcY, -1);
                QVector4D vVector= (eyeDirection * eyeZ).toVector4D();
                vVector.setW(1);

                //QVector4D vVector= projMatrix.inverted() * clipPos;

                QVector4D wVector= (viewMatrix * modelMatrix).inverted() * vVector;

                out << wVector.x() << " " << wVector.y() << " " << wVector.z() << "\n";
            }
        }
        file.close();
    }
    first++;
    */
