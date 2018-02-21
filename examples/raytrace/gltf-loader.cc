#include "gltf-loader.h"

#include <iostream>
#include <memory>  // c++11
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

using std::out_of_range;

namespace example {

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

///
/// Loads glTF 2.0 mesh
///
bool LoadGLTF(const std::string &filename, float scale,
              std::vector<Mesh<float> > *meshes,
              std::vector<Material> *materials,
              std::vector<Texture> *textures) {
  // TODO(syoyo): Implement
  // TODO(syoyo): Texture
  // TODO(syoyo): Material

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string ext = GetFilePathExtension(filename);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    // assume binary glTF.
    ret = loader.LoadBinaryFromFile(&model, &err, filename.c_str());
  } else {
    // assume ascii glTF.
    ret = loader.LoadASCIIFromFile(&model, &err, filename.c_str());
  }

  if (!err.empty()) {
    std::cerr << "glTF parse error: " << err << std::endl;
  }
  if (!ret) {
    std::cerr << "Failed to load glTF: " << filename << std::endl;
    return false;
  }

  std::cout << "loaded glTF file has:\n"
            << model.accessors.size() << " accessors\n"
            << model.animations.size() << " animations\n"
            << model.buffers.size() << " buffers\n"
            << model.bufferViews.size() << " bufferViews\n"
            << model.materials.size() << " materials\n"
            << model.meshes.size() << " meshes\n"
            << model.nodes.size() << " nodes\n"
            << model.textures.size() << " textures\n"
            << model.images.size() << " images\n"
            << model.skins.size() << " skins\n"
            << model.samplers.size() << " samplers\n"
            << model.cameras.size() << " cameras\n"
            << model.scenes.size() << " scenes\n"
            << model.lights.size() << " lights\n";

  for (const auto &gltfMesh : model.meshes) {
    std::cout << "Current mesh has " << gltfMesh.primitives.size()
              << " primitives:\n";

    Mesh<float> loadedMesh(sizeof(float) * 3);

    v3f pMin = {}, pMax = {};

    loadedMesh.name = gltfMesh.name;
    for (const auto &meshPrimitive : gltfMesh.primitives) {
      // get access to the indices
      std::unique_ptr<intArrayBase> indicesArrayPtr = nullptr;
      {
        auto &indicesAccessor = model.accessors[meshPrimitive.indices];
        auto &bufferView = model.bufferViews[indicesAccessor.bufferView];
        auto &buffer = model.buffers[bufferView.buffer];
        auto dataAddress = buffer.data.data() + bufferView.byteOffset +
                           indicesAccessor.byteOffset;
        const auto byteStride = indicesAccessor.ByteStride(bufferView);
        const auto count = indicesAccessor.count;

        // Allocate the index array in the pointer-to-base declared in the
        // parent scope
        switch (indicesAccessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            indicesArrayPtr =
                std::unique_ptr<intArray<char> >(new intArray<char>(
                    arrayAdapter<char>(dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            indicesArrayPtr = std::unique_ptr<intArray<unsigned char> >(
                new intArray<unsigned char>(arrayAdapter<unsigned char>(
                    dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_SHORT:
            indicesArrayPtr =
                std::unique_ptr<intArray<short> >(new intArray<short>(
                    arrayAdapter<short>(dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            indicesArrayPtr = std::unique_ptr<intArray<unsigned short> >(
                new intArray<unsigned short>(arrayAdapter<unsigned short>(
                    dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_INT:
            indicesArrayPtr = std::unique_ptr<intArray<int> >(new intArray<int>(
                arrayAdapter<int>(dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            indicesArrayPtr = std::unique_ptr<intArray<unsigned int> >(
                new intArray<unsigned int>(arrayAdapter<unsigned int>(
                    dataAddress, count, byteStride)));
            break;
          default:
            break;
        }
      }

      // Get access to the index array. Don't worry about underlying type
      const auto &indices = *indicesArrayPtr;

      if (indicesArrayPtr)
        for (size_t i(0); i < indices.size(); ++i) {
          loadedMesh.faces.push_back(indices[i]);
          std::cout << indices[i] << " ";
        }

      std::cout << '\n';

      switch (meshPrimitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:  // this is the simpliest case to handle

        {
          std::cout << "Will load a plain old list of trianges\n";

          for (const auto &attribute : meshPrimitive.attributes) {
            const auto attribAccessor = model.accessors[attribute.second];
            const auto &bufferView =
                model.bufferViews[attribAccessor.bufferView];
            const auto &buffer = model.buffers[bufferView.buffer];
            auto dataPtr = buffer.data.data() + bufferView.byteOffset +
                           attribAccessor.byteOffset;
            const auto byte_stride = attribAccessor.ByteStride(bufferView);
            const auto count = attribAccessor.count;

            if (attribute.first == "POSITION") {
              std::cout << "found position attribute\n";

              // get the position min/max for computing the boundingbox
              pMin.x = attribAccessor.minValues[0];
              pMin.y = attribAccessor.minValues[1];
              pMin.z = attribAccessor.minValues[2];
              pMax.x = attribAccessor.maxValues[0];
              pMax.y = attribAccessor.maxValues[1];
              pMax.z = attribAccessor.maxValues[2];

              switch (attribAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                  switch (attribAccessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                      // 3D vector of float
                      v3fArray positions(
                          arrayAdapter<v3f>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < positions.size(); ++i) {
                        const auto v = positions[i];
                        std::cout << '(' << v.x << ", " << v.y << ", " << v.z
                                  << ")\n";

                        loadedMesh.vertices.push_back(v.x * scale);
                        loadedMesh.vertices.push_back(v.y * scale);
                        loadedMesh.vertices.push_back(v.z * scale);
                      }
                    } break;
                    default:
                      // TODO Handle error
                      break;
                  }
                case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                  switch (attribAccessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                      v3dArray positions(
                          arrayAdapter<v3d>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < positions.size(); ++i) {
                        const auto v = positions[i];
                        std::cout << '(' << v.x << ", " << v.y << ", " << v.z
                                  << ")\n";

                        loadedMesh.vertices.push_back(v.x * scale);
                        loadedMesh.vertices.push_back(v.y * scale);
                        loadedMesh.vertices.push_back(v.z * scale);
                      }
                    } break;
                    default:
                      break;
                  }
                } break;
                default:
                  // TODO handle error
                  break;
              }
            }

            if (attribute.first == "NORMAL") {
              std::cout << "found normal attribute\n";

              switch (attribAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                  switch (attribAccessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                      std::cout << "normal vec3\n";
                      v3fArray normals(
                          arrayAdapter<v3f>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < normals.size(); ++i) {
                        const auto v = normals[i];
                        std::cout << '(' << v.x << ", " << v.y << ", " << v.z
                                  << ")\n";

                        loadedMesh.facevarying_normals.push_back(v.x);
                        loadedMesh.facevarying_normals.push_back(v.y);
                        loadedMesh.facevarying_normals.push_back(v.z);
                      }
                    } break;
                    case TINYGLTF_TYPE_VEC4:
                      std::cout << "normal vec4";
                      break;
                    default:
                      // TODO handle error
                      break;
                  }
                default:
                  // TODO handle error
                  break;
              }
            }

            if (attribute.first == "TEXCOORD_0") {
              std::cout << "Found texture coordinates\n";

              switch (attribAccessor.type) {
                case TINYGLTF_TYPE_VEC2: {
                  switch (attribAccessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                      v2fArray uvs(
                          arrayAdapter<v2f>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < uvs.size(); ++i) {
                        const auto v = uvs[i];
                        std::cout << '(' << v.x << ", " << v.y << ")\n";

                        loadedMesh.facevarying_uvs.push_back(v.x);
                        loadedMesh.facevarying_uvs.push_back(v.y);
                      }
                    } break;
                    case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                      v2dArray uvs(
                          arrayAdapter<v2d>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < uvs.size(); ++i) {
                        const auto v = uvs[i];
                        std::cout << '(' << v.x << ", " << v.y << ")\n";

                        loadedMesh.facevarying_uvs.push_back(v.x);
                        loadedMesh.facevarying_uvs.push_back(v.y);
                      }
                    } break;
                    default:
                      break;
                  }
                } break;
                default:
                  break;
              }
            }
          }
        }

        break;

        // Other trigangle based modes
        case TINYGLTF_MODE_TRIANGLE_FAN:
        case TINYGLTF_MODE_TRIANGLE_STRIP:
        default:
          std::cerr << "primitive mode not implemented";
          break;

        // These aren't triangles:
        case TINYGLTF_MODE_POINTS:
        case TINYGLTF_MODE_LINE:
        case TINYGLTF_MODE_LINE_LOOP:
          std::cerr << "primitive is not triangle based, ignoring";
      }
    }

    // TODO compute pivot point

    // bbox :
    v3f bCenter;
    bCenter.x = 0.5f * (pMax.x - pMin.x) + pMin.x;
    bCenter.y = 0.5f * (pMax.y - pMin.y) + pMin.y;
    bCenter.z = 0.5f * (pMax.z - pMin.z) + pMin.z;

    for (size_t v = 0; v < loadedMesh.vertices.size() / 3; v++) {
      loadedMesh.vertices[3 * v + 0] -= bCenter.x;
      loadedMesh.vertices[3 * v + 1] -= bCenter.y;
      loadedMesh.vertices[3 * v + 2] -= bCenter.z;
    }

    loadedMesh.pivot_xform[0][0] = 1.0f;
    loadedMesh.pivot_xform[0][1] = 0.0f;
    loadedMesh.pivot_xform[0][2] = 0.0f;
    loadedMesh.pivot_xform[0][3] = 0.0f;

    loadedMesh.pivot_xform[1][0] = 0.0f;
    loadedMesh.pivot_xform[1][1] = 1.0f;
    loadedMesh.pivot_xform[1][2] = 0.0f;
    loadedMesh.pivot_xform[1][3] = 0.0f;

    loadedMesh.pivot_xform[2][0] = 0.0f;
    loadedMesh.pivot_xform[2][1] = 0.0f;
    loadedMesh.pivot_xform[2][2] = 1.0f;
    loadedMesh.pivot_xform[2][3] = 0.0f;

    loadedMesh.pivot_xform[3][0] = bCenter.x;
    loadedMesh.pivot_xform[3][1] = bCenter.y;
    loadedMesh.pivot_xform[3][2] = bCenter.z;
    loadedMesh.pivot_xform[3][3] = 1.0f;

    for (size_t i{0}; i < loadedMesh.faces.size(); ++i)
      loadedMesh.material_ids.push_back(materials->at(0).id);

    meshes->push_back(loadedMesh);
    ret = true;
  }
  // std::cerr << "LoadGLTF() function is not yet implemented!" << std::endl;
  return ret;
}
}  // namespace example
