#include <iostream>
#include <sstream>
#include <math.h>
#include <stdlib.h>
#include <osg/Notify>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <osgDB/FileNameUtils>
#include <osgDB/Registry>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <iomanip>

using namespace tinygltf;


class GLTFReader: public osgDB::ReaderWriter
{
    public:
        GLTFReader()
        {
            supportsExtension("gltf","glTF ascii loader");
            supportsExtension("glb","glTF binary loader");
        }

        virtual const char* className() const { return "glTF Loader"; }

        osg::Node* makeMesh(tinygltf::Model &model, tinygltf::Mesh& mesh) const
        {
            osg::ref_ptr< osg::Geometry > geom = new osg::Geometry;
            osg::Vec4Array* colors = new osg::Vec4Array;
            colors->push_back(osg::Vec4(1,1,1,1));
            geom->setColorArray(colors, osg::Array::BIND_OVERALL);

            for (size_t i = 0; i < mesh.primitives.size(); i++) {
                const tinygltf::Primitive &primitive = mesh.primitives[i];
                if (primitive.indices < 0)
                {
                    return 0;
                }

                OSG_NOTICE << "Material index " << primitive.material << std::endl;
                if (primitive.material >= 0)
                {                    
                    OSG_NOTICE << "Getting material " << primitive.material << std::endl;
                    tinygltf::Material& material = model.materials[primitive.material];
                    OSG_NOTICE << "Material " << material.name << std::endl;

                    OSG_NOTICE << "extCommonValues=" << material.extCommonValues.size() << std::endl;
                    for (ParameterMap::iterator paramItr = material.extCommonValues.begin(); paramItr != material.extCommonValues.end(); ++paramItr)
                    {
                        OSG_NOTICE << paramItr->first << "=" << paramItr->second.string_value << std::endl;
                    }

                    OSG_NOTICE << "additionalValues=" << material.additionalValues.size() << std::endl;
                    for (ParameterMap::iterator paramItr = material.additionalValues.begin(); paramItr != material.additionalValues.end(); ++paramItr)
                    {
                        OSG_NOTICE << paramItr->first << "=" << paramItr->second.string_value << std::endl;
                    }
                    OSG_NOTICE << "extPBRValues=" << material.extPBRValues.size() << std::endl;
                    for (ParameterMap::iterator paramItr = material.extPBRValues.begin(); paramItr != material.extPBRValues.end(); ++paramItr)
                    {
                        OSG_NOTICE << paramItr->first << "=" << paramItr->second.string_value << std::endl;
                    } 
                    OSG_NOTICE << "values=" << material.values.size() << std::endl;
                    for (ParameterMap::iterator paramItr = material.values.begin(); paramItr != material.values.end(); ++paramItr)
                    {
                        if (paramItr->first == "baseColorTexture")
                        {
                            int index = paramItr->second.json_double_value["index"];
                            tinygltf::Texture& texture = model.textures[index];
                            Sampler& sampler = model.samplers[texture.sampler];

                            osg::Texture2D* tex  = new osg::Texture2D;
                            tex->setFilter(osg::Texture::MIN_FILTER, (osg::Texture::FilterMode)sampler.minFilter);
                            tex->setFilter(osg::Texture::MAG_FILTER, (osg::Texture::FilterMode)sampler.magFilter);
                            tex->setWrap(osg::Texture::WRAP_S, (osg::Texture::WrapMode)sampler.wrapS);
                            tex->setWrap(osg::Texture::WRAP_T, (osg::Texture::WrapMode)sampler.wrapT);
                            tex->setWrap(osg::Texture::WRAP_R, (osg::Texture::WrapMode)sampler.wrapR);

                            tinygltf::Image& image = model.images[texture.source];
                            osg::ref_ptr< osg::Image> img = new osg::Image;

                            GLenum format = GL_RGB;
                            if (image.component == 4) format = GL_RGBA;

                            if (image.image.size() > 0)
                            {
                                unsigned char *imgData = new unsigned char[image.image.size()];
                                memcpy(imgData, &image.image.at(0), image.image.size());
                                img->setImage(image.width, image.height, 1, format, format, GL_UNSIGNED_BYTE, imgData, osg::Image::AllocationMode::USE_NEW_DELETE);
                            }   
                            
                            tex->setImage( img );
                            geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
                        }                        
                    } 
                }

                std::map<std::string, int>::const_iterator it(primitive.attributes.begin());
                std::map<std::string, int>::const_iterator itEnd(
                    primitive.attributes.end());

                for (; it != itEnd; it++) {
                    const tinygltf::Accessor &accessor = model.accessors[it->second];


                    int size = 1;
                    if (accessor.type == TINYGLTF_TYPE_SCALAR) {
                        size = 1;
                    } else if (accessor.type == TINYGLTF_TYPE_VEC2) {
                        size = 2;
                    } else if (accessor.type == TINYGLTF_TYPE_VEC3) {
                        size = 3;
                    } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
                        size = 4;
                    } else {
                        assert(0);
                    }
                    
                    OSG_NOTICE << it->first << std::endl;

                    if (it->first.compare("POSITION") == 0 && size == 3)
                    {                        
                        BufferView& bufferView = model.bufferViews[accessor.bufferView];
                        Buffer& buffer = model.buffers[bufferView.buffer];
                        osg::Vec3Array* vertices = new osg::Vec3Array;
                        geom->setVertexArray( vertices );

                        float* array = (float*)(&buffer.data.at(0) + accessor.byteOffset + bufferView.byteOffset);
                        unsigned int pos = 0;
                        for (unsigned int j = 0; j < accessor.count; j++)
                        {
                            float x = array[pos];
                            float y = array[pos+1];
                            float z = array[pos+2];  
                            if (bufferView.byteStride > 0)
                            {
                                pos += (bufferView.byteStride / 4);
                            }
                            else
                            {
                                pos += 3;
                            }
                            vertices->push_back(osg::Vec3(x,y,z));
                        }
                    }    

                    if (it->first.compare("NORMAL") == 0 && size == 3)
                    {                

                        BufferView& bufferView = model.bufferViews[accessor.bufferView];
                        Buffer& buffer = model.buffers[bufferView.buffer];
                        osg::Vec3Array* normals = new osg::Vec3Array;
                        geom->setNormalArray( normals, osg::Array::BIND_PER_VERTEX );

                        float* array = (float*)(&buffer.data.at(0) + accessor.byteOffset + bufferView.byteOffset);
                        unsigned int pos = 0;
                        for (unsigned int j = 0; j < accessor.count; j++)
                        {
                            float x = array[pos];
                            float y = array[pos+1];
                            float z = array[pos+2];  
                            if (bufferView.byteStride > 0)
                            {
                                pos += (bufferView.byteStride / 4);
                            }
                            else
                            {
                                pos += 3;
                            }
                            normals->push_back(osg::Vec3(x,y,z));
                        }
                    }

                    if (it->first.compare("TEXCOORD_0") == 0 && size == 2)
                    {                        
                        BufferView& bufferView = model.bufferViews[accessor.bufferView];
                        Buffer& buffer = model.buffers[bufferView.buffer];
                        osg::Vec2Array* texCoords = new osg::Vec2Array;
                        geom->setTexCoordArray(0, texCoords, osg::Array::BIND_PER_VERTEX );

                        float* array = (float*)(&buffer.data.at(0) + accessor.byteOffset + bufferView.byteOffset);
                        unsigned int pos = 0;
                        for (unsigned int j = 0; j < accessor.count; j++)
                        {
                            float s = array[pos];
                            float t = array[pos+1];
                            if (bufferView.byteStride > 0)
                            {
                                pos += (bufferView.byteStride / 4);
                            }
                            else
                            {
                                pos += 2;
                            }
                            texCoords->push_back(osg::Vec2(s,t));
                        }
                    } 
                }

                const tinygltf::Accessor &indexAccessor =
                    model.accessors[primitive.indices];

                int mode = -1;
                if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
                    mode = GL_TRIANGLES;
                } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
                    mode = GL_TRIANGLE_STRIP;
                } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
                    mode = GL_TRIANGLE_FAN;
                } else if (primitive.mode == TINYGLTF_MODE_POINTS) {
                    mode = GL_POINTS;
                } else if (primitive.mode == TINYGLTF_MODE_LINE) {
                    mode = GL_LINES;
                } else if (primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
                    mode = GL_LINE_LOOP;
                }

                {
                    BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
                    Buffer& buffer = model.buffers[bufferView.buffer];

                    if (indexAccessor.componentType == GL_UNSIGNED_SHORT)
                    {
                        osg::DrawElementsUShort* drawElements = new osg::DrawElementsUShort(mode);
                        unsigned short* indices = (unsigned short*)(&buffer.data.at(0) + bufferView.byteOffset + indexAccessor.byteOffset);
                        for (unsigned int j = 0; j < indexAccessor.count; j++)
                        {
                            unsigned short index = indices[j];
                            drawElements->push_back( index );
                        }      
                        geom->addPrimitiveSet(drawElements);
                    }
                    else if (indexAccessor.componentType == GL_UNSIGNED_INT)
                    {
                        osg::DrawElementsUInt* drawElements = new osg::DrawElementsUInt(mode);
                        unsigned int* indices = (unsigned int*)(&buffer.data.at(0) + bufferView.byteOffset + indexAccessor.byteOffset);
                        for (unsigned int j = 0; j < indexAccessor.count; j++)
                        {
                            unsigned int index = indices[j];
                            drawElements->push_back( index );
                        }                            
                        geom->addPrimitiveSet(drawElements);
                    }
                    else if (indexAccessor.componentType == GL_UNSIGNED_BYTE)
                    {
                        osg::DrawElementsUByte* drawElements = new osg::DrawElementsUByte(mode);
                        unsigned char* indices = (unsigned char*)(&buffer.data.at(0) + bufferView.byteOffset + indexAccessor.byteOffset);
                        for (unsigned int j = 0; j < indexAccessor.count; j++)
                        {
                            unsigned char index = indices[j];
                            drawElements->push_back( index );
                        }                            
                        geom->addPrimitiveSet(drawElements);
                    }                        
                }

            }


            return geom.release();
        }


        osg::Node* createNode(tinygltf::Model &model, tinygltf::Node& node) const
        {
            osg::MatrixTransform* mt = new osg::MatrixTransform;
            mt->setName( node.name );
            if (node.matrix.size() == 16)
            {
                osg::Matrixd mat;
                mat.set(node.matrix.data());
                mt->setMatrix(mat);
            }
            else
            {
                osg::Matrixd scale, translation, rotation;
                if (node.scale.size() == 3)
                {
                    scale = osg::Matrixd::scale(node.scale[0], node.scale[1], node.scale[2]);
                }

                if (node.rotation.size() == 4) {
                    rotation = osg::Matrixd::rotate(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
                }

                if (node.translation.size() == 3) {
                    translation = osg::Matrixd::translate(node.translation[0], node.translation[1], node.translation[2]);
                }

                mt->setMatrix( scale * rotation * translation );
            }


            // todo transformation
            if (node.mesh >= 0)
            {
                mt->addChild( makeMesh(model, model.meshes[node.mesh]) );
            }

            // Load any children.
            for (unsigned int i = 0; i < node.children.size(); i++)
            {
                osg::Node* child = createNode(model, model.nodes[ node.children[i] ]);
                if (child)
                {
                    mt->addChild(child);
                }
            }
            return mt;
        }

        osg::Node* makeNodeFromModel(tinygltf::Model &model) const
        {
            // TODO:  Read all the scenes

            // Get the default scene
            const tinygltf::Scene &scene = model.scenes[0];

            osg::Group* group = new osg::Group;

            // Load all of the nodes in the scene.
            for (size_t i = 0; i < scene.nodes.size(); i++) {
                osg::Node* node = createNode(model, model.nodes[scene.nodes[i]]);
                if (node)
                {
                    group->addChild(node);
                }
            }

            return group;
        }



        virtual ReadResult readObject(const std::string& fileName, const Options* opt) const
        { return readNode(fileName,opt); }

        virtual ReadResult readNode(const std::string& fileName, const Options* options) const
        {
            std::string ext = osgDB::getFileExtension(fileName);
            if (!acceptsExtension(ext))
                return ReadResult::FILE_NOT_HANDLED;        

            Model model; 
            TinyGLTF loader;
            std::string err;

            OSG_NOTICE << fileName << std::endl;
            
            if (ext == "glb")
            {
                loader.LoadBinaryFromFile(&model, &err, fileName);
            }
            else
            {
                loader.LoadASCIIFromFile(&model, &err, fileName);
            }
            if (!err.empty()) {
                OSG_NOTICE << "gltf Error loading " << fileName << std::endl;
                OSG_WARN << err << std::endl;
                return ReadResult::ERROR_IN_READING_FILE;
            }

            return makeNodeFromModel( model );
        }    
};

REGISTER_OSGPLUGIN(gltf, GLTFReader)
