#include <stdlib.h>
#include <iostream>
#include <eigen3/Eigen/Dense>
#include <chrono>
#include <cmath>
#include <string>
#include<list>

using namespace Eigen;



//matrices to use here and there
class UsefulMatrices{
    public:
    static Matrix3f GenRotationMatX(float cosAngle, float sinAngle){
        Matrix3f mat;
        mat << 1, 0, 0,
               0, cosAngle, -sinAngle,
               0, sinAngle, cosAngle;
        return mat;
    }

    static Matrix3f GenRotationMatY(float cosAngle, float sinAngle){
        Matrix3f mat;
        mat << cosAngle, 0, sinAngle, 
               0, 1, 0,
               -sinAngle, 0, cosAngle;
        return mat;
    }

    static Matrix3f GenRotationMatZ(float cosAngle, float sinAngle){
        Matrix3f mat;
        mat << cosAngle, -sinAngle, 0,
               sinAngle, cosAngle, 0,
               0, 0, 1;
        return mat;
    }

    static Matrix4f GenTranslationMatrix(float xTranslate, float yTranslate, float zTranslate){
        Matrix4f mat;
        mat << 1, 0, 0, xTranslate,
               0, 1, 0, yTranslate,
               0, 0, 1, zTranslate,
               0, 0, 0, 1;
        return mat;
    }

    static Matrix3f Matrix4x4toMat3x3(Matrix4f gotMat){
        Matrix3f mat;
        mat << gotMat(0,0), gotMat(0,1), gotMat(0,2),
               gotMat(1,0), gotMat(1,1), gotMat(1,2),
               gotMat(2,0), gotMat(2,1), gotMat(2,2);
        return mat;
    }

    static Matrix4f Matrix3x3toHomo(Matrix3f gotMat){
        Matrix4f mat;
        mat <<  gotMat(0,0), gotMat(0,1), gotMat(0,2), 0,
                gotMat(1,0), gotMat(1,1), gotMat(1,2), 0,
                gotMat(2,0), gotMat(2,1), gotMat(2,2), 0,
                0, 0, 0, 1;
        return mat;
        }

    static Matrix4f GenDonnutingMatrix(float ringAngle, float tubeAngle, float translation){
        Matrix4f mat;
        float cosRingAngle = cos(ringAngle); float sinRingAngle = sin(ringAngle);
        float cosTubeAngle = cos(tubeAngle); float sinTubeAngle = sin(tubeAngle);


        mat = Matrix3x3toHomo(GenRotationMatZ(cosRingAngle, sinRingAngle))*
              GenTranslationMatrix(translation, 0, 0)*
              Matrix3x3toHomo(GenRotationMatY(cosTubeAngle, sinTubeAngle))*
              GenTranslationMatrix(-translation, 0, 0);
        return mat;
    }
        


};


//the class with information about the donut
class Donut{
public: 
    float innerWidth;
    float tubeWidth;
    float tubeSpacing;
    float ringSpacing;

    Donut(float innerWidth, float tubeWidth, float tubeSpacing, float ringSpacing){
        this->innerWidth = innerWidth;
        this->ringSpacing = ringSpacing;
        this->tubeSpacing = tubeSpacing;
        this->tubeWidth = tubeWidth;
    }
};


//contains the buffers for rendering and info about them
class RenderTexture{
public: 
    int pixelsWidth;
    int pixelsHeight;
    float eyeDist;
    float screenPos;

    std::vector<char> frameBuffer;
        std::vector<float> Zbuffer;

    RenderTexture(int pixelsWidth, int pixelsHeight, float eyeDist, float screenZ){
        this->screenPos = screenZ;
        this->eyeDist = eyeDist;
        this->pixelsHeight = pixelsHeight;
        this->pixelsWidth = pixelsWidth;
        this->Zbuffer.resize(this->pixelsHeight*this->pixelsWidth);
        this->frameBuffer.resize(this->pixelsHeight*this->pixelsWidth);

        
        for (auto& thing : this->Zbuffer){
            thing = 99999;
        }

        for (auto& thing: this->frameBuffer){
            thing = ' ';
        }
    }

    int GetBufferIndexFromCoords(int x, int y){
        if (-pixelsWidth/2 <= x && x < pixelsWidth/2 && -pixelsHeight/2 <= y && y < pixelsHeight/2)
            return ((y+pixelsHeight/2)*pixelsWidth + x+pixelsWidth/2);
        else
            return -1;
    }
};


//here the rendering actually happens
int main(int argc, char* argv[]) {
    

    float in = 10;
    float tube = 5;
    float tubeSpace = 0.3;
    float ringSpace = 0.08;
    float speedx = 3;
    float speedy = 1;
    float speedz = 2;
    float screenZ = -20;
    float eyeDist = 100;
    float width = 50;
    float height = 50;

    std::vector<float*> args = {&in, &tube, &tubeSpace, &ringSpace, &speedx, &speedy, &speedz, &screenZ, &eyeDist, &width, &height};

    if (argc>1){
        for(int i = 1; i<argc; i++){
            *args[i-1] = std::stof(argv[i]);
        }
    }







    Donut donut(in, tube, tubeSpace, ringSpace);
    RenderTexture screen(width, height, eyeDist, screenZ);

    Vector3f rotAngles;
    Vector3f baseVec(donut.innerWidth, 0, 0);
    Vector3f baseNorm(-1,0,0);
    

    float time;
    float lighting;
    Vector3f donuttedPoint;
    Vector3f transformedPoint;
    Vector4f homoPoint;
    Vector2f screenCoords;
    Vector3f sunVec(0,-1,0);
    Vector3f normal;
    
    Matrix3f rotations;


    auto startTime = std::chrono::steady_clock::now();


    std::list<Vector3f> donuttedPointsList;
    std::list<Vector3f> donuttedNormalsList;


    for (float j = 0; j< 2*M_PI; j+=donut.ringSpacing){
        for (float i = 0; i < 2*M_PI; i+=donut.tubeSpacing){
            Matrix4f donuttingMat = UsefulMatrices::GenDonnutingMatrix(j, i, donut.innerWidth+donut.tubeWidth);
            donuttedPointsList.push_back((donuttingMat*baseVec.homogeneous()).head<3>());
            donuttedNormalsList.push_back((donuttingMat*baseNorm.homogeneous()).head<3>().normalized());
        }
    }

    std::vector<Vector3f> donuttedPoints(donuttedPointsList.begin(), donuttedPointsList.end());
    std::vector<Vector3f> donuttedNormals(donuttedNormalsList.begin(), donuttedNormalsList.end());


    int pointCount = donuttedPointsList.size();


    //frame loop
    while (true){
            
        time = (std::chrono::steady_clock::now() - startTime).count()/1000000000.0;
        Vector3f Angles(time*speedx, time*speedy, time*speedz);

        float cosX = cos(Angles.x()); float sinX = sin(Angles.x());
        float cosY = cos(Angles.y()); float sinY = sin(Angles.y());
        float cosZ = cos(Angles.z()); float sinZ = sin(Angles.z());
        
        rotations = UsefulMatrices::GenRotationMatZ(cosZ, sinZ)*
                    UsefulMatrices::GenRotationMatY(cosY, sinY)*
                    UsefulMatrices::GenRotationMatX(cosX, sinX);




        for (auto& i : screen.Zbuffer)
            i = 99999;

        for (auto& i : screen.frameBuffer)
            i = ' ';


        





        for (int i = 0; i< pointCount; i++){
            transformedPoint = rotations * donuttedPoints[i];
            normal = rotations * donuttedNormals[i];
            lighting = (sunVec.dot(normal) + 1)/2;


                
            float relZ = transformedPoint.z() -(screen.screenPos-screen.eyeDist);
            if (relZ > 0){
                screenCoords = (transformedPoint.head<2>() * (screen.eyeDist)/relZ);
                int bufferIndex = screen.GetBufferIndexFromCoords((int)screenCoords.x(), (int)screenCoords.y());

                if(transformedPoint.z() < screen.Zbuffer[bufferIndex]){
                    screen.Zbuffer[bufferIndex] = transformedPoint.z();
                    screen.frameBuffer[bufferIndex] = ".,-~:;=!*#$@"[(int)(lighting*10)];
                }
            
            }
        }

        system("clear");
        
        std::cout << "\n\n\n";        
        
        for(int y = 0; y < screen.pixelsHeight; y++){
            for(int x = 0; x< screen.pixelsWidth; x++){
                std::cout << screen.frameBuffer[y*screen.pixelsWidth + x];
            }
            std::cout<<std::endl;
        }
        
        while (((std::chrono::steady_clock::now() - startTime).count()/1000000000.0)-time < 1/60.0){
            continue;
        }
        std::cout << 1/((std::chrono::steady_clock::now() - startTime).count()/1000000000.0-time);


    }



    return 0;
}
