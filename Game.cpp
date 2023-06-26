// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"
#include "Plane.hpp"
#include "UserInputs.hpp"

// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)

struct MeshUniformBlock {
	alignas(4) float amb;
	alignas(4) float gamma;
	alignas(16) glm::vec3 sColor;
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct OverlayUniformBlock {
	alignas(4) float visible;
};

struct GlobalUniformBlock {
	alignas(16) glm::vec3 DlightDir;
	alignas(16) glm::vec3 DlightColor;
	alignas(16) glm::vec3 AmbLightColor;
	alignas(16) glm::vec3 eyePos;
};

// The vertices data structures
struct VertexMesh {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

struct VertexOverlay {
	glm::vec2 pos;
	glm::vec2 UV;
};

// MAIN ! 
class Game : public BaseProject {
	protected:

	// Current aspect ratio (used by the callback that resized the window
	float Ar;

	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLGubo, DSLMesh, DSLOverlay;

	// Vertex formats
	VertexDescriptor VMesh;
	VertexDescriptor VOverlay;

	// Pipelines [Shader couples]
	Pipeline PMesh;
	Pipeline POverlay;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	Model<VertexMesh> MPlane, MArrow, MBox; /** one per model **/
	std::array<Model<VertexMesh>, 4> MPark;
	Model<VertexOverlay> MKey, MSplash;
	DescriptorSet DSGubo, DSPlane, DSArrow, DSBox, DSKey, DSSplash; /** one per instance of model **/
	std::array<DescriptorSet, 4> DSPark;
	Texture TCity, TArrow, TKey, TSplash;
	
	// C++ storage for uniform variables
	MeshUniformBlock uboPlane, uboArrow, uboBox;
    std::array<MeshUniformBlock, 4> uboPark;
	GlobalUniformBlock gubo;
	OverlayUniformBlock uboKey, uboSplash;

	int gameState;

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Drone delivery";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 10;
		texturesInPool = 9;
		setsInPool = 10;
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}
	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLMesh.init(this, {
					// this array contains the bindings:
					// first  element : the binding number
					// second element : the type of element (buffer or texture)
					//                  using the corresponding Vulkan constant
					// third  element : the pipeline stage where it will be used
					//                  using the corresponding Vulkan constant
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				});
				
		DSLOverlay.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				});				
		DSLGubo.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
				});

		// Vertex descriptors
		VMesh.init(this, {
				  // this array contains the bindings
				  // first  element : the binding number
				  // second element : the stride of this binging
				  // third  element : whether this parameter change per vertex or per instance
				  //                  using the corresponding Vulkan constant
				  {0, sizeof(VertexMesh), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  // this array contains the location
				  // first  element : the binding number
				  // second element : the location number
				  // third  element : the offset of this element in the memory record
				  // fourth element : the data type of the element
				  //                  using the corresponding Vulkan constant
				  // fifth  elmenet : the size in byte of the element
				  // sixth  element : a constant defining the element usage
				  //                   POSITION - a vec3 with the position
				  //                   NORMAL   - a vec3 with the normal vector
				  //                   UV       - a vec2 with a UV coordinate
				  //                   COLOR    - a vec4 with a RGBA color
				  //                   TANGENT  - a vec4 with the tangent vector
				  //                   OTHER    - anything else
				  //
				  // ***************** DOUBLE CHECK ********************
				  //    That the Vertex data structure you use in the "offsetoff" and
				  //	in the "sizeof" in the previous array, refers to the correct one,
				  //	if you have more than one vertex format!
				  // ***************************************************
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexMesh, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexMesh, norm),
				         sizeof(glm::vec3), NORMAL},
				  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexMesh, UV),
				         sizeof(glm::vec2), UV}
				});

		VOverlay.init(this, {
				  {0, sizeof(VertexOverlay), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, pos),
				         sizeof(glm::vec2), OTHER},
				  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, UV),
				         sizeof(glm::vec2), UV}
				});

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		PMesh.init(this, &VMesh, "shaders/MeshVert.spv", "shaders/MeshFrag.spv", {&DSLGubo, &DSLMesh});
		POverlay.init(this, &VOverlay, "shaders/OverlayVert.spv", "shaders/OverlayFrag.spv", {&DSLOverlay});
		POverlay.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
 								    VK_CULL_MODE_NONE, false);

		// Models, textures and Descriptors (values assigned to the uniforms)

		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
        for (int i = 0; i < MPark.size(); ++i) {
            std::string modelFile = "Models/park_00" + std::to_string(i + 1) + ".mgcg";
            MPark[i].init(this, &VMesh, modelFile, MGCG);
        }

		MPlane.init(this, &VMesh, "Models/plane_001.mgcg", MGCG);
        MArrow.init(this, &VMesh, "Models/arrow.obj", OBJ);
        MBox.init(this, &VMesh, "Models/box_005.mgcg", MGCG);

		// Creates a mesh with direct enumeration of vertices and indices
		MKey.vertices = {{{-0.8f, 0.6f}, {0.0f,0.0f}}, {{-0.8f, 0.95f}, {0.0f,1.0f}},
						 {{ 0.8f, 0.6f}, {1.0f,0.0f}}, {{ 0.8f, 0.95f}, {1.0f,1.0f}}};
		MKey.indices = {0, 1, 2,    1, 2, 3};
		MKey.initMesh(this, &VOverlay);
		
		// Creates a mesh with direct enumeration of vertices and indices
		MSplash.vertices = {{{-1.0f, -0.58559f}, {0.0102f, 0.0f}}, {{-1.0f, 0.58559f}, {0.0102f,0.85512f}},
						 {{ 1.0f,-0.58559f}, {1.0f,0.0f}}, {{ 1.0f, 0.58559f}, {1.0f,0.85512f}}};
		MSplash.indices = {0, 1, 2,    1, 2, 3};
		MSplash.initMesh(this, &VOverlay);
		
		// Create the textures
		// The second parameter is the file name
		TCity.init(this, "textures/Textures_City.png");
        TArrow.init(this, "textures/arrow.png");
		TKey.init(this,    "textures/PressSpace.png");
		TSplash.init(this, "textures/SplashScreen.png");

		gameState = 0;
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		PMesh.create();
		POverlay.create();

        for (auto &dsPark : DSPark) {
            dsPark.init(this, &DSLMesh, {
                {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}});
        }

		DSPlane.init(this, &DSLMesh, {
					{0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
					{1, TEXTURE, 0, &TCity}
				});
        DSArrow.init(this, &DSLMesh, {
                {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
                {1, TEXTURE, 0, &TArrow}
        });
        DSBox.init(this, &DSLMesh, {
                {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}
        });
		DSKey.init(this, &DSLOverlay, {
					{0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
					{1, TEXTURE, 0, &TKey}
				});
		DSSplash.init(this, &DSLOverlay, {
					{0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
					{1, TEXTURE, 0, &TSplash}
				});
		DSGubo.init(this, &DSLGubo, {
					{0, UNIFORM, sizeof(GlobalUniformBlock), nullptr}
				});
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		PMesh.cleanup();
		POverlay.cleanup();

		// Cleanup datasets
        for (auto &dsPark : DSPark) {
            dsPark.cleanup();
        }

		DSPlane.cleanup();
        DSBox.cleanup();
        DSArrow.cleanup();
		DSKey.cleanup();
		DSSplash.cleanup();
		DSGubo.cleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {
		// Cleanup textures
		TCity.cleanup();
        TArrow.cleanup();
		TKey.cleanup();
		TSplash.cleanup();
		
		// Cleanup models
        for (auto &mPark : MPark) {
            mPark.cleanup();
        }

		MPlane.cleanup();
        MBox.cleanup();
        MArrow.cleanup();
		MKey.cleanup();
		MSplash.cleanup();
		
		// Cleanup descriptor set layouts
		DSLMesh.cleanup();
		DSLOverlay.cleanup();

		DSLGubo.cleanup();
		
		// Destroies the pipelines
		PMesh.destroy();		
		POverlay.destroy();
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		// sets global uniforms (see below fro parameters explanation)
		DSGubo.bind(commandBuffer, PMesh, 0, currentImage);

		// binds the pipeline
		PMesh.bind(commandBuffer);
		// For a pipeline object, this command binds the corresponing pipeline to the command buffer passed in its parameter

		// binds the model
        for (int i = 0; i < MPark.size(); ++i) {
            MPark[i].bind(commandBuffer);
            DSPark[i].bind(commandBuffer, PMesh, 1, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MPark[i].indices.size()), 1, 0, 0, 0);
        }

        MPlane.bind(commandBuffer);
		DSPlane.bind(commandBuffer, PMesh, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPlane.indices.size()), 1, 0, 0, 0);

        MArrow.bind(commandBuffer);
        DSArrow.bind(commandBuffer, PMesh, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MArrow.indices.size()), 1, 0, 0, 0);

        MBox.bind(commandBuffer);
        DSBox.bind(commandBuffer, PMesh, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MBox.indices.size()), 1, 0, 0, 0);

		POverlay.bind(commandBuffer);
		MKey.bind(commandBuffer);
		DSKey.bind(commandBuffer, POverlay, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MKey.indices.size()), 1, 0, 0, 0);

		MSplash.bind(commandBuffer);
		DSSplash.bind(commandBuffer, POverlay, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MSplash.indices.size()), 1, 0, 0, 0);
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		// Standard procedure to quit when the ESC key is pressed
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

        /**
         * keep gubo.eyePos fixed
         * compute the position of the plane, i.e. the plane's world matrix + view and projection matrices based on plane position (world matrix)
         * terrain world matrix stays fixed (as already computed) and vp matrices are same as vp of plane
         *
         * ==> only thing that needs computing is WVP matrix of the plane, all others act accordingly
         */

        auto userInputs = UserInputs(this);

		switch(gameState) {		// main state machine implementation
		  case 0: // initial state - show splash screen
			if(userInputs.handleFire) {
				gameState = 1;	// jump to the wait key state
			}
			break;
		  case 1: // wait key state
          /*
			if(userInputs.handleFire) {
				gameState = 0;	// jump back to splash screen
			} */
			break;
		}

        const float camHeight = 0.25;
        const float camDist = 25.0;
        const float camPitch = 0.1f;
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.f;

        static auto* const plane = new Plane(userInputs);


        /**
         * MPark[0].vertices access map vertices for collision detection: finding top 3 closest vertices to player not enough
         * because you can't know if the condition to enforce is player.xyz >< terrain.xyz,
         * but you can find the vertex "terrain" with closest xz and enforce that player.y > terrain.y
         */

        glm::mat4 worldMat = plane->computeWorldMatrix();
        glm::vec3 camPos = computeCameraPosition(worldMat, camDist, camHeight, camPitch);
        glm::vec3 planePos = plane->getPositionInWorldCoordinates();
        glm::mat4 viewMat = glm::lookAt(camPos, planePos, glm::vec3(0.0f, 1.0f, 0.0f)) ;
        glm::mat4 projMat = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        projMat[1][1] *= -1;

		gubo.DlightDir = glm::normalize(glm::vec3(1, 2, 3));
		gubo.DlightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.AmbLightColor = glm::vec3(0.1f);
		gubo.eyePos = glm::vec3(100.0, 100.0, 100.0);
		// Writes value to the GPU
		DSGubo.map(currentImage, &gubo, sizeof(gubo), 0);
		// the .map() method of a DataSet object, requires the current image of the swap chain as first parameter
		// the second parameter is the pointer to the C++ data structure to transfer to the GPU
		// the third parameter is its size
		// the fourth parameter is the location inside the descriptor set of this uniform block

        /**
         * It's actually the world that moves around, while the camera stays fixed, so each object in the world
         * has its own Model View Projection matrix (mvpMat), as you see below, and they all move using the World matrix
         */

        static glm::mat4 parkWorldMat = glm::mat4(1);
        for (int i = 0; i < uboPark.size(); ++i) {
            glm::vec3 trasl = {0, 0, 0};
            if (i == 1) trasl = {16.0, 0, 0};
            if (i == 2) trasl = {0, 0, 16.0};
            if (i == 3) trasl = {16.0, 0, 16.0};
            parkWorldMat = glm::translate(glm::mat4(1), trasl);
            uboPark[i].amb = 1.0f; uboPark[i].gamma = 180.0f; uboPark[i].sColor = glm::vec3(1.0f);
            uboPark[i].mvpMat = projMat * viewMat * parkWorldMat;
            uboPark[i].mMat = parkWorldMat;
            uboPark[i].nMat = glm::inverse(glm::transpose(parkWorldMat));
            DSPark[i].map(currentImage, &uboPark[i], sizeof(uboPark[i]), 0);
        }

        uboPlane.amb = 1.0f; uboPlane.gamma = 180.0f; uboPlane.sColor = glm::vec3(1.0f);
        uboPlane.mvpMat = projMat * viewMat * worldMat;
        uboPlane.mMat = worldMat;
        uboPlane.nMat = glm::inverse(glm::transpose(worldMat));
		DSPlane.map(currentImage, &uboPlane, sizeof(uboPlane), 0);

        static glm::mat4 arrowWorldMat = glm::translate(parkWorldMat, glm::vec3{3, 5, 3});
        const int RANGE = 10;
        const int START = -5;
        if (userInputs.handleFire) {
            auto x = static_cast<float>(rand() % RANGE + START) ;
            auto z = static_cast<float>(rand() % RANGE + START) ;
            arrowWorldMat = glm::translate(arrowWorldMat, glm::vec3{x, 0, z});
        }
        uboArrow.amb = 1.0f; uboArrow.gamma = 180.0f; uboArrow.sColor = glm::vec3(1.0f);
        uboArrow.mvpMat = projMat * viewMat * arrowWorldMat;
        uboArrow.mMat = arrowWorldMat;
        uboArrow.nMat = glm::inverse(glm::transpose(arrowWorldMat));
        DSArrow.map(currentImage, &uboArrow, sizeof(uboArrow), 0);

        uboBox.amb = 1.0f; uboBox.gamma = 180.0f; uboBox.sColor = glm::vec3(1.0f);
        uboBox.mvpMat = projMat * viewMat * parkWorldMat;
        uboBox.mMat = parkWorldMat;
        uboBox.nMat = glm::inverse(glm::transpose(parkWorldMat));
        DSBox.map(currentImage, &uboBox, sizeof(uboBox), 0);

		uboKey.visible = (gameState == 1) ? 1.0f : 0.0f;
		DSKey.map(currentImage, &uboKey, sizeof(uboKey), 0);

		uboSplash.visible = (gameState == 0) ? 1.0f : 0.0f;
		DSSplash.map(currentImage, &uboSplash, sizeof(uboSplash), 0);
	}

    /**
    * @param world world transform matrix
    * @param camDistance distance from tracked object in object's coordinates
    * @param camHeight height from tracked object in object's coordinates
    * @param camPitch pitch in radians
    * @return camera position in world coordinates
    */
    static vec3 computeCameraPosition(const mat4& world, float camDistance, float camHeight, float camPitch) {
        const float MIN_CAM_HEIGHT_WORLD_COORDINATES = 0.5;

        vec3 position =
                world
                * glm::rotate(glm::mat4(1.0f), glm::radians(- 90.0f), glm::vec3(0,1,0))
                * glm::vec4(- camDistance * std::cos(camPitch),
                          camHeight + camDistance * std::sin(camPitch),
                          0.0f,
                          1);

        position.y = std::max(position.y, 0.5f); // avoids camera from going below the ground level
        return position;
    }
};


// This is the main: probably you do not need to touch this!
int main() {
    Game app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}