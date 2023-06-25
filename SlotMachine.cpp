// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"

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
class SlotMachine : public BaseProject {
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
	Model<VertexMesh> MPark1, MPark2, MPark3, MPark4, MHandle; /** one per model **/
	Model<VertexOverlay> MKey, MSplash;
	DescriptorSet DSGubo, DSPark1, DSPark2, DSPark3, DSPark4, DSHandle, DSKey, DSSplash; /** one per instance of model **/
	Texture TCity, THandle, TKey, TSplash;
	
	// C++ storage for uniform variables
	MeshUniformBlock uboPark1, uboPark2, uboPark3, uboPark4, uboHandle;
	GlobalUniformBlock gubo;
	OverlayUniformBlock uboKey, uboSplash;

	// Other application parameters
	float CamH, CamRadius, CamPitch, CamYaw;
	int gameState;
	float HandleRot = 0.0;
	float Wheel1Rot = 0.0;
	float Wheel2Rot = 0.0;
	float Wheel3Rot = 0.0;

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Slot Machine";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 8;
		texturesInPool = 7;
		setsInPool = 8;
		
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
		MPark1.init(this, &VMesh, "Models/park_001.mgcg", MGCG);
        MPark2.init(this, &VMesh, "Models/park_002.mgcg", MGCG);
        MPark3.init(this, &VMesh, "Models/park_003.mgcg", MGCG);
        MPark4.init(this, &VMesh, "Models/park_004.mgcg", MGCG);

		MHandle.init(this, &VMesh, "Models/SlotHandle.obj", OBJ);

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
		THandle.init(this, "textures/SlotHandle.png");
		TKey.init(this,    "textures/PressSpace.png");
		TSplash.init(this, "textures/SplashScreen.png");
		
		// Init local variables
		CamH = 1.0f;
		CamRadius = 3.0f;
		CamPitch = glm::radians(15.f);
		CamYaw = glm::radians(30.f);
		gameState = 0;
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		PMesh.create();
		POverlay.create();
		
		// Here you define the data set
		DSPark1.init(this, &DSLMesh, {
		// the second parameter, is a pointer to the Uniform Set Layout of this set
		// the last parameter is an array, with one element per binding of the set.
		// first  elmenet : the binding number
		// second element : UNIFORM or TEXTURE (an enum) depending on the type
		// third  element : only for UNIFORMs, the size of the corresponding C++ object. For texture, just put 0
		// fourth element : only for TEXTUREs, the pointer to the corresponding texture object. For uniforms, use nullptr
					{0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
					{1, TEXTURE, 0, &TCity}
				});
        DSPark2.init(this, &DSLMesh, {
                {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}
        });
        DSPark3.init(this, &DSLMesh, {
                {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}
        });
        DSPark4.init(this, &DSLMesh, {
                {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}
        });

		DSHandle.init(this, &DSLMesh, {
					{0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
					{1, TEXTURE, 0, &THandle}
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
		DSPark1.cleanup();
        DSPark2.cleanup();
        DSPark3.cleanup();
        DSPark4.cleanup();

		DSHandle.cleanup();

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
		THandle.cleanup();
		TKey.cleanup();
		TSplash.cleanup();
		
		// Cleanup models
		MPark1.cleanup();
        MPark2.cleanup();
        MPark3.cleanup();
        MPark4.cleanup();

		MHandle.cleanup();
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
		MPark1.bind(commandBuffer);
		// For a Model object, this command binds the corresponing index and vertex buffer
		// to the command buffer passed in its parameter
		
		// binds the data set
		DSPark1.bind(commandBuffer, PMesh, 1, currentImage);
		// For a Dataset object, this command binds the corresponing dataset
		// to the command buffer and pipeline passed in its first and second parameters.
		// The third parameter is the number of the set being bound
		// As described in the Vulkan tutorial, a different dataset is required for each image in the swap chain.
		// This is done automatically in file Starter.hpp, however the command here needs also the index
		// of the current image in the swap chain, passed in its last parameter
					
		// record the drawing command in the command buffer
		vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPark1.indices.size()), 1, 0, 0, 0);
		// the second parameter is the number of indexes to be drawn. For a Model object,
		// this can be retrieved with the .indices.size() method.

        MPark2.bind(commandBuffer);
        DSPark2.bind(commandBuffer, PMesh, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPark2.indices.size()), 1, 0, 0, 0);

        MPark3.bind(commandBuffer);
        DSPark3.bind(commandBuffer, PMesh, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPark3.indices.size()), 1, 0, 0, 0);

        MPark4.bind(commandBuffer);
        DSPark4.bind(commandBuffer, PMesh, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPark4.indices.size()), 1, 0, 0, 0);

        MHandle.bind(commandBuffer);
		DSHandle.bind(commandBuffer, PMesh, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MHandle.indices.size()), 1, 0, 0, 0);

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
		
		// Integration with the timers and the controllers
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);
		// getSixAxis() is defined in Starter.hpp in the base class.
		// It fills the float point variable passed in its first parameter with the time
		// since the last call to the procedure.
		// It fills vec3 in the second parameters, with three values in the -1,1 range corresponding
		// to motion (with left stick of the gamepad, or ASWD + RF keys on the keyboard)
		// It fills vec3 in the third parameters, with three values in the -1,1 range corresponding
		// to motion (with right stick of the gamepad, or Arrow keys + QE keys on the keyboard, or mouse)
		// If fills the last boolean variable with true if fire has been pressed:
		//          SPACE on the keyboard, A or B button on the Gamepad, Right mouse button

		// To debounce the pressing of the fire button, and start the event when the key is released
		static bool wasFire = false;
		bool handleFire = (wasFire && (!fire));
		wasFire = fire;
		
		// Parameters: wheels and handle speed and range
		const float HandleSpeed = glm::radians(90.0f);
		const float HandleRange = glm::radians(45.0f);
		const float WheelSpeed = glm::radians(180.0f);
		const float SymExtent = glm::radians(15.0f);	// size of one symbol on the wheel in angle rad.
		// static variables for current angles
		static float TargetRot = 0.0;	// Target rotation

		switch(gameState) {		// main state machine implementation
		  case 0: // initial state - show splash screen
			if(handleFire) {
				gameState = 1;	// jump to the wait key state
			}
			break;
		  case 1: // wait key state
			if(handleFire) {
				gameState = 2;	// jump to the moving handle state
			}
			break;
		  case 2: // handle moving down state
			HandleRot += HandleSpeed * deltaT;
			Wheel1Rot += WheelSpeed * deltaT;
			Wheel2Rot += WheelSpeed * deltaT;
			Wheel3Rot += WheelSpeed * deltaT;
			if(HandleRot > HandleRange) {	// when limit is reached, jump the handle moving up state
				gameState = 3;
				HandleRot = HandleRange;
			}
			break;
		  case 3: // handle moving up state
			HandleRot -= HandleSpeed * deltaT;
			Wheel1Rot += WheelSpeed * deltaT;
			Wheel2Rot += WheelSpeed * deltaT;
			Wheel3Rot += WheelSpeed * deltaT;
			if(HandleRot < 0.0f) {	// when limit is reached, jump the 3 wheels spinning state
				gameState = 4;
				HandleRot = 0.0f;
				TargetRot = Wheel1Rot + (10 + (rand() % 11)) * SymExtent;
			}
			break;
		  case 4: // 3 wheels spinning state
			Wheel1Rot += WheelSpeed * deltaT;
			Wheel2Rot += WheelSpeed * deltaT;
			Wheel3Rot += WheelSpeed * deltaT;
			if(Wheel1Rot >= TargetRot) {	// When the target rotation is reached, jump to the next state
				gameState = 5;
				Wheel1Rot = round(TargetRot / SymExtent) * SymExtent; // quantize position
				TargetRot = Wheel2Rot + (10 + (rand() % 11)) * SymExtent;
			}
			break;
		  case 5: // 2 wheels spinning state
			Wheel2Rot += WheelSpeed * deltaT;
			Wheel3Rot += WheelSpeed * deltaT;
			if(Wheel2Rot >= TargetRot) {	// When the target rotation is reached, jump to the next state
				gameState = 6;
				Wheel2Rot = round(TargetRot / SymExtent) * SymExtent; // quantize position
				TargetRot = Wheel3Rot + (10 + (rand() % 11)) * SymExtent;
			}
			break;
		  case 6: // 1 wheels spinning state
			Wheel3Rot += WheelSpeed * deltaT;
			if(Wheel3Rot >= TargetRot) {	// When the target rotation is reached, jump to the next state
				gameState = 1;
				Wheel3Rot = round(TargetRot / SymExtent) * SymExtent; // quantize position
			}
			break;
		}
		
		// Parameters
		// Camera FOV-y, Near Plane and Far Plane
		const float FOVy = glm::radians(90.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 100.0f;
		const float rotSpeed = glm::radians(90.0f);
		const float movSpeed = 1.0f;
		
		CamH += m.z * movSpeed * deltaT;
		CamRadius -= m.x * movSpeed * deltaT;
		CamPitch -= r.x * rotSpeed * deltaT;
		CamYaw += r.y * rotSpeed * deltaT;
		
		glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
		Prj[1][1] *= -1;
		glm::vec3 camTarget = glm::vec3(0,CamH,0);
		glm::vec3 camPos    = camTarget +
							  CamRadius * glm::vec3(cos(CamPitch) * sin(CamYaw),
													sin(CamPitch),
													cos(CamPitch) * cos(CamYaw));
		glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));

		gubo.DlightDir = glm::normalize(glm::vec3(1, 2, 3));
		gubo.DlightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.AmbLightColor = glm::vec3(0.1f);
		gubo.eyePos = camPos;

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

        glm::mat4 World = glm::mat4(1);
        uboPark1.amb = 1.0f; uboPark1.gamma = 180.0f; uboPark1.sColor = glm::vec3(1.0f);
        uboPark1.mvpMat = Prj * View * World;
        uboPark1.mMat = World;
        uboPark1.nMat = glm::inverse(glm::transpose(World));
		DSPark1.map(currentImage, &uboPark1, sizeof(uboPark1), 0);

        World = glm::translate(glm::mat4(1), glm::vec3{2.0, 2.0, 2.0});
        uboPark2.amb = 1.0f; uboPark2.gamma = 180.0f; uboPark2.sColor = glm::vec3(1.0f);
        uboPark2.mvpMat = Prj * View * World;
        uboPark2.mMat = World;
        uboPark2.nMat = glm::inverse(glm::transpose(World));
        DSPark2.map(currentImage, &uboPark2, sizeof(uboPark2), 0);

        glm::mat4(1);
        uboPark3.amb = 1.0f; uboPark3.gamma = 180.0f; uboPark3.sColor = glm::vec3(1.0f);
        uboPark3.mvpMat = Prj * View * World;
        uboPark3.mMat = World;
        uboPark3.nMat = glm::inverse(glm::transpose(World));
        DSPark3.map(currentImage, &uboPark3, sizeof(uboPark3), 0);

        glm::mat4(1);
        uboPark4.amb = 1.0f; uboPark4.gamma = 180.0f; uboPark4.sColor = glm::vec3(1.0f);
        uboPark4.mvpMat = Prj * View * World;
        uboPark4.mMat = World;
        uboPark4.nMat = glm::inverse(glm::transpose(World));
        DSPark4.map(currentImage, &uboPark4, sizeof(uboPark4), 0);
	
		World = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.3f,0.5f,-0.15f)),
							HandleRot, glm::vec3(1,0,0));
		uboHandle.amb = 1.0f; uboHandle.gamma = 180.0f; uboHandle.sColor = glm::vec3(1.0f);
		uboHandle.mvpMat = Prj * View * World;
		uboHandle.mMat = World;
		uboHandle.nMat = glm::inverse(glm::transpose(World));
		DSHandle.map(currentImage, &uboHandle, sizeof(uboHandle), 0);

		uboKey.visible = (gameState == 1) ? 1.0f : 0.0f;
		DSKey.map(currentImage, &uboKey, sizeof(uboKey), 0);

		uboSplash.visible = (gameState == 0) ? 1.0f : 0.0f;
		DSSplash.map(currentImage, &uboSplash, sizeof(uboSplash), 0);
	}	
};


// This is the main: probably you do not need to touch this!
int main() {
    SlotMachine app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}