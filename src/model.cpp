#include "model.h"

// Assimp is only used in this file. The rest of the program sees the plain Model struct.
#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <map>

#include "texture.h"


// Everything the texture loading needs while one model is read
struct TextureLoader {
  const aiScene* scene;

  // The folder of the model file, image files are looked up from there
  std::filesystem::path folder;

  // True for .gltf and .glb files, where image names are written like web addresses
  bool isGltf = false;

  // Textures already loaded, by their name in the file and their settings, so an image
  // used several times with the same settings is loaded once
  std::map<std::string, GLuint> loaded;

  // The 1 x 1 white texture for missing textures, made when first needed
  GLuint white = 0;
};


// Turn escapes such as "%23" in a glTF image name back into the character they stand for
// ("%23" is '#', "%20" is a space). Anything that is not a valid escape is kept as it is.
static std::string decodePercent(const std::string& text) {
  std::string result;
  for (size_t i = 0; i < text.size(); i++) {
    if (text[i] == '%' && i + 2 < text.size() && std::isxdigit((unsigned char)text[i + 1]) &&
        std::isxdigit((unsigned char)text[i + 2])) {
      result += (char)std::stoi(text.substr(i + 1, 2), nullptr, 16);
      i += 2;
    } else {
      result += text[i];
    }
  }
  return result;
}


// Turn an Assimp wrap mode into the OpenGL one
// glTF only has repeat, clamp and mirror. Decal comes from other formats and is treated as clamp.
static GLint wrapMode(int mode) {
  if (mode == aiTextureMapMode_Clamp || mode == aiTextureMapMode_Decal) {
    return GL_CLAMP_TO_EDGE;
  }
  if (mode == aiTextureMapMode_Mirror) {
    return GL_MIRRORED_REPEAT;
  }
  return GL_REPEAT;
}


// Read how one texture of a material wants to be sampled
// glTF stores the filters with the OpenGL numbers, so they can be used as they are.
// Anything missing or unknown keeps the default from TextureSettings.
static TextureSettings readTextureSettings(const aiMaterial* material, aiTextureType type) {
  TextureSettings settings;
  int value;
  if (material->Get(AI_MATKEY_MAPPINGMODE_U(type, 0), value) == AI_SUCCESS) {
    settings.wrapS = wrapMode(value);
  }
  if (material->Get(AI_MATKEY_MAPPINGMODE_V(type, 0), value) == AI_SUCCESS) {
    settings.wrapT = wrapMode(value);
  }
  if (material->Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(type, 0), value) == AI_SUCCESS) {
    if (value == GL_NEAREST || value == GL_LINEAR) {
      settings.magFilter = value;
    }
  }
  if (material->Get(AI_MATKEY_GLTF_MAPPINGFILTER_MIN(type, 0), value) == AI_SUCCESS) {
    if (value == GL_NEAREST || value == GL_LINEAR || value == GL_NEAREST_MIPMAP_NEAREST ||
        value == GL_LINEAR_MIPMAP_NEAREST || value == GL_NEAREST_MIPMAP_LINEAR ||
        value == GL_LINEAR_MIPMAP_LINEAR) {
      settings.minFilter = value;
    }
  }
  return settings;
}


// Load the first texture of one type from a material
// The id is 0 if the material has no texture of that type or the image could not be read.
// srgb is true for textures that hold colors, see TextureSettings.
static MaterialTexture loadMaterialTexture(TextureLoader& loader, Model& model, const aiMaterial* material,
                                           aiTextureType type, bool srgb) {
  MaterialTexture result;
  aiString name;
  if (material->GetTexture(type, 0, &name) != AI_SUCCESS) {
    return result;
  }

  // Which set of texture coordinates the texture uses. The mesh keeps sets 0 and 1.
  int uvSet;
  if (material->Get(AI_MATKEY_UVWSRC(type, 0), uvSet) == AI_SUCCESS) {
    result.uvSet = uvSet;
  }
  if (result.uvSet < 0 || result.uvSet > 1) {
    std::printf("  Warning: texture %s uses texture coordinates %d, only 0 and 1 are kept, using 0\n",
                name.C_Str(), result.uvSet);
    result.uvSet = 0;
  }
  result.settings = readTextureSettings(material, type);
  result.settings.srgb = srgb;

  // The same image with other settings is a separate texture, because OpenGL keeps
  // the settings and the format in the texture itself
  std::string imageName = name.C_Str();
  std::string key = imageName + " " + std::to_string(result.settings.wrapS) + " " +
                    std::to_string(result.settings.wrapT) + " " + std::to_string(result.settings.minFilter) +
                    " " + std::to_string(result.settings.magFilter) + " " +
                    (result.settings.srgb ? "srgb" : "plain");
  if (loader.loaded.count(key) > 0) {
    result.id = loader.loaded[key];
    return result;
  }

  // Images inside the file (as in .glb) are named "*0", "*1" and so on.
  // Other images are files, normally next to the model file.
  GLuint texture = 0;
  const aiTexture* embedded = loader.scene->GetEmbeddedTexture(name.C_Str());
  if (embedded != nullptr && embedded->mHeight == 0) {
    // A compressed image such as PNG or JPEG, mWidth is its size in bytes
    texture = loadTextureMemory((const unsigned char*)embedded->pcData, (int)embedded->mWidth, imageName,
                                result.settings);
  } else if (embedded != nullptr) {
    // Plain pixels, which Assimp stores in the order blue, green, red, alpha
    std::vector<unsigned char> pixels;
    for (unsigned int i = 0; i < embedded->mWidth * embedded->mHeight; i++) {
      pixels.push_back(embedded->pcData[i].r);
      pixels.push_back(embedded->pcData[i].g);
      pixels.push_back(embedded->pcData[i].b);
      pixels.push_back(embedded->pcData[i].a);
    }
    std::printf("  Texture %s (inside the file): %u x %u pixels\n", imageName.c_str(), embedded->mWidth,
                embedded->mHeight);
    texture = createTexture(pixels.data(), (int)embedded->mWidth, (int)embedded->mHeight, result.settings);
  } else {
    // glTF writes image names like web addresses, so "%23" stands for '#'.
    // OBJ and FBX names are plain file names and are used as they are.
    std::string fileName = imageName;
    if (loader.isGltf) {
      fileName = decodePercent(imageName);
    }
    // A path that starts at a drive or at the top folder (as some OBJ and FBX files have)
    // is used as it is. Any other path is relative to the model's folder.
    std::filesystem::path imagePath(fileName);
    if (!imagePath.has_root_path()) {
      imagePath = loader.folder / imagePath;
    }
    texture = loadTextureFile(imagePath.string(), result.settings);
  }

  loader.loaded[key] = texture;
  if (texture != 0) {
    model.textures.push_back(texture);
  }
  result.id = texture;
  return result;
}


// Print a warning when a drawn texture uses something the viewer does not apply yet
static void warnAboutTexture(const aiMaterial* material, aiTextureType type, const char* what) {
  // A texture transform (the glTF KHR_texture_transform extension) moves, turns or scales
  // the texture coordinates of one texture
  const aiMaterialProperty* property = nullptr;
  if (aiGetMaterialProperty(material, AI_MATKEY_UVTRANSFORM(type, 0), &property) == AI_SUCCESS) {
    std::printf("  Warning: the %s texture has a texture transform, which is not applied\n", what);
  }
}


// Print the coordinate set, wrap S and T, and min and mag filter of a drawn texture, as OpenGL
// numbers (10497 repeat, 33071 clamp to edge, 33648 mirrored repeat, 9728 nearest, 9729 linear)
static void printTexture(const char* what, const MaterialTexture& t) {
  if (t.id != 0) {
    std::printf("    %s texture: coordinates %d, wrap %d %d, filter %d %d, %s\n", what, t.uvSet,
                t.settings.wrapS, t.settings.wrapT, t.settings.minFilter, t.settings.magFilter,
                t.settings.srgb ? "sRGB" : "plain");
  }
}


// The white texture, made the first time it is needed
static GLuint whiteTexture(TextureLoader& loader, Model& model) {
  if (loader.white == 0) {
    loader.white = createWhiteTexture();
    model.textures.push_back(loader.white);
  }
  return loader.white;
}


// Read the values and textures of one material
// glTF files give every value. OBJ and FBX files often have no metallic or roughness,
// so those get a non-metal with medium roughness.
static Material readMaterial(TextureLoader& loader, Model& model, const aiMaterial* source) {
  Material material;

  // Base color: glTF has its own key, the other formats only have the diffuse color
  aiColor4D color;
  if (source->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
      source->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
    material.baseColorFactor = glm::vec4(color.r, color.g, color.b, color.a);
  }

  float value;
  material.metallicFactor = 0.0f;
  if (source->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS) {
    material.metallicFactor = value;
  }
  material.roughnessFactor = 0.5f;
  if (source->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS) {
    material.roughnessFactor = value;
  }

  aiColor3D emissive;
  if (source->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
    material.emissiveFactor = glm::vec3(emissive.r, emissive.g, emissive.b);
  }
  if (source->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), value) == AI_SUCCESS) {
    material.normalScale = value;
  }
  if (source->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), value) == AI_SUCCESS) {
    material.occlusionStrength = value;
  }

  // Alpha mode is the text "OPAQUE", "MASK" or "BLEND" in glTF
  aiString alphaMode;
  if (source->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
    std::string mode = alphaMode.C_Str();
    if (mode == "MASK") {
      material.alphaMode = ALPHA_MASK;
    } else if (mode == "BLEND") {
      material.alphaMode = ALPHA_BLEND;
    }
  }
  if (source->Get(AI_MATKEY_GLTF_ALPHACUTOFF, value) == AI_SUCCESS) {
    material.alphaCutoff = value;
  }
  int twoSided;
  if (source->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS) {
    material.doubleSided = (twoSided != 0);
  }

  // Textures. Assimp files the glTF occlusion texture under "lightmap".
  aiTextureType baseColorType = aiTextureType_BASE_COLOR;
  // Base color and emissive hold colors (sRGB), the others hold data
  material.baseColorTexture = loadMaterialTexture(loader, model, source, baseColorType, true);
  if (material.baseColorTexture.id == 0) {
    baseColorType = aiTextureType_DIFFUSE;
    material.baseColorTexture = loadMaterialTexture(loader, model, source, baseColorType, true);
  }
  material.metallicRoughnessTexture =
      loadMaterialTexture(loader, model, source, aiTextureType_GLTF_METALLIC_ROUGHNESS, false);
  warnAboutTexture(source, baseColorType, "base color");
  warnAboutTexture(source, aiTextureType_GLTF_METALLIC_ROUGHNESS, "metallic-roughness");
  material.normalTexture = loadMaterialTexture(loader, model, source, aiTextureType_NORMALS, false);
  material.occlusionTexture = loadMaterialTexture(loader, model, source, aiTextureType_LIGHTMAP, false);
  material.emissiveTexture = loadMaterialTexture(loader, model, source, aiTextureType_EMISSIVE, true);

  // Print what was found before the missing textures are replaced by white
  std::printf("  Material \"%s\": base color (%.2f %.2f %.2f %.2f), metallic %.2f, roughness %.2f\n",
              source->GetName().C_Str(), material.baseColorFactor.r, material.baseColorFactor.g,
              material.baseColorFactor.b, material.baseColorFactor.a, material.metallicFactor,
              material.roughnessFactor);
  std::printf("    textures: base color %s, metallic-roughness %s, normal %s, occlusion %s, emissive %s\n",
              material.baseColorTexture.id ? "yes" : "no", material.metallicRoughnessTexture.id ? "yes" : "no",
              material.normalTexture.id ? "yes" : "no", material.occlusionTexture.id ? "yes" : "no",
              material.emissiveTexture.id ? "yes" : "no");
  printTexture("base color", material.baseColorTexture);
  printTexture("metallic-roughness", material.metallicRoughnessTexture);

  // A missing texture becomes white, so texture times factor gives just the factor.
  // The normal map stays 0, because white would tilt the normals.
  if (material.baseColorTexture.id == 0) {
    material.baseColorTexture.id = whiteTexture(loader, model);
  }
  if (material.metallicRoughnessTexture.id == 0) {
    material.metallicRoughnessTexture.id = whiteTexture(loader, model);
  }
  if (material.occlusionTexture.id == 0) {
    material.occlusionTexture.id = whiteTexture(loader, model);
  }
  if (material.emissiveTexture.id == 0) {
    material.emissiveTexture.id = whiteTexture(loader, model);
  }
  return material;
}


// Turn one Assimp mesh into our vertex layout and upload it
// Returns false if the mesh has no triangles (only points or lines).
static bool readMesh(const aiMesh* source, Mesh& mesh, glm::vec3& boundsMin, glm::vec3& boundsMax,
                     bool& firstVertex) {
  std::vector<unsigned int> indices;
  for (unsigned int i = 0; i < source->mNumFaces; i++) {
    const aiFace& face = source->mFaces[i];
    if (face.mNumIndices == 3) {
      indices.push_back(face.mIndices[0]);
      indices.push_back(face.mIndices[1]);
      indices.push_back(face.mIndices[2]);
    }
  }
  if (indices.empty()) {
    return false;
  }

  std::vector<float> vertices;
  for (unsigned int i = 0; i < source->mNumVertices; i++) {
    glm::vec3 position(source->mVertices[i].x, source->mVertices[i].y, source->mVertices[i].z);

    // Normals are always there because we ask Assimp to make them if the file has none
    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    if (source->HasNormals()) {
      normal = glm::vec3(source->mNormals[i].x, source->mNormals[i].y, source->mNormals[i].z);
    }

    // The first two sets of texture coordinates are kept
    // Without a second set, the first one is copied, so the shader can always read both.
    glm::vec2 uv(0.0f);
    if (source->HasTextureCoords(0)) {
      uv = glm::vec2(source->mTextureCoords[0][i].x, source->mTextureCoords[0][i].y);
    }
    glm::vec2 uv1 = uv;
    if (source->HasTextureCoords(1)) {
      uv1 = glm::vec2(source->mTextureCoords[1][i].x, source->mTextureCoords[1][i].y);
    }

    // Assimp gives a tangent and a bitangent. We keep the tangent and store in the fourth
    // number whether the bitangent is cross(normal, tangent) (1) or the opposite (-1).
    // Without texture coordinates there are no tangents, so any direction will do.
    glm::vec4 tangent(1.0f, 0.0f, 0.0f, 1.0f);
    if (source->HasTangentsAndBitangents()) {
      glm::vec3 t(source->mTangents[i].x, source->mTangents[i].y, source->mTangents[i].z);
      glm::vec3 b(source->mBitangents[i].x, source->mBitangents[i].y, source->mBitangents[i].z);
      float side = (glm::dot(glm::cross(normal, t), b) < 0.0f) ? -1.0f : 1.0f;
      tangent = glm::vec4(t, side);
    }

    float vertex[FLOATS_PER_VERTEX] = { position.x, position.y, position.z, normal.x, normal.y, normal.z,
                                        uv.x, uv.y, tangent.x, tangent.y, tangent.z, tangent.w,
                                        uv1.x, uv1.y };
    vertices.insert(vertices.end(), vertex, vertex + FLOATS_PER_VERTEX);

    // Grow the box around the model
    if (firstVertex) {
      boundsMin = position;
      boundsMax = position;
      firstVertex = false;
    }
    boundsMin = glm::min(boundsMin, position);
    boundsMax = glm::max(boundsMax, position);
  }

  mesh = createMesh(vertices, indices);
  std::printf("  Mesh \"%s\": %u vertices, %d triangles, material %u\n", source->mName.C_Str(),
              source->mNumVertices, (int)indices.size() / 3, source->mMaterialIndex);
  return true;
}


// Read a model file
Model loadModel(const std::string& path) {
  Model model;
  std::printf("Loading model %s\n", path.c_str());

  // What Assimp does to the data after reading it:
  // - Triangulate: split polygons with more than three corners into triangles
  // - GenSmoothNormals: make normals if the file has none
  // - JoinIdenticalVertices: share vertices between triangles so the index buffer is used
  // - CalcTangentSpace: work out tangents from the texture coordinates
  // - PreTransformVertices: apply the node transforms, so all meshes sit where the file puts them
  // - FlipUVs: Assimp hands out texture coordinates with V counting from the bottom of the image,
  //   for every format (it turns the glTF ones around to get there). Flipping them makes V count
  //   from the top again, which matches images uploaded first row first, as we do.
  //   So the images themselves are never flipped.
  unsigned int steps = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
                       aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_FlipUVs;

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(path, steps);
  if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || scene->mRootNode == nullptr) {
    model.error = importer.GetErrorString();
    if (model.error.empty()) {
      // Assimp does not always say why, for example when the file is only partly read
      model.error = "the file is incomplete or broken";
    }
    std::printf("Could not load the model %s: %s\n", path.c_str(), model.error.c_str());
    return model;
  }

  // Materials first, because each mesh names its material by number
  TextureLoader loader;
  loader.scene = scene;
  loader.folder = std::filesystem::path(path).parent_path();
  std::string extension = std::filesystem::path(path).extension().string();
  for (char& c : extension) {
    c = (char)std::tolower((unsigned char)c);
  }
  loader.isGltf = (extension == ".gltf" || extension == ".glb");
  std::vector<Material> materials;
  for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
    materials.push_back(readMaterial(loader, model, scene->mMaterials[i]));
  }

  // Then the meshes, each with its material
  bool firstVertex = true;
  for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    const aiMesh* source = scene->mMeshes[i];
    ModelPart part;
    if (!readMesh(source, part.mesh, model.boundsMin, model.boundsMax, firstVertex)) {
      std::printf("  Skipping mesh \"%s\", it has no triangles\n", source->mName.C_Str());
      continue;
    }
    part.material = materials[source->mMaterialIndex];
    model.parts.push_back(part);
  }

  if (model.parts.empty()) {
    std::printf("Could not load the model %s: it has no triangles\n", path.c_str());
    deleteModel(model);
    model.error = "it has no triangles";
    return model;
  }

  std::printf("Loaded %d meshes, %u materials, %d textures (images inside the file: %u)\n",
              (int)model.parts.size(), scene->mNumMaterials, (int)model.textures.size(), scene->mNumTextures);
  std::printf("Bounds from (%.3f %.3f %.3f) to (%.3f %.3f %.3f)\n", model.boundsMin.x, model.boundsMin.y,
              model.boundsMin.z, model.boundsMax.x, model.boundsMax.y, model.boundsMax.z);
  return model;
}


// Free the buffers and textures of a model
void deleteModel(Model& model) {
  for (ModelPart& part : model.parts) {
    deleteMesh(part.mesh);
  }
  if (!model.textures.empty()) {
    glDeleteTextures((GLsizei)model.textures.size(), model.textures.data());
  }
  model.parts.clear();
  model.textures.clear();
}
