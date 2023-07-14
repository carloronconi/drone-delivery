// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"
#include "Plane.hpp"
#include "UserInputs.hpp"
#include "Package.hpp"
#include "DataStructs.hpp"
#include "UserModelPool.hpp"

// MAIN ! 
class Game : public BaseProject {
	protected:

	// Current aspect ratio (used by the callback that resized the window
	float Ar;

	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLGubo, DSLMetallic, DSLOpaque, DSLEmit, DSLOverlay, DSLPropeller;

	// Vertex formats
	VertexDescriptor VClassic, VOverlay, VAnimation;

	// Pipelines [Shader couples]
	Pipeline PMetallic, POpaque, PEmit, POverlay, PPropeller;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	Model<VertexClassic> MPlane, MArrow; /** one per model **/
	Model<VertexClassic> MBox, MGround;
	std::array<Model<VertexClassic>, 12> MCity;
    Model<VertexClassic> MRoad, MStreet; /** use instanced-rendering **/
	Model<VertexOverlay> MScore, MLife, MSplash, MWin, MLose, MHelp; /** score and life use instanced-rendering **/
	Model<VertexAnimation> MPropeller;
	DescriptorSet DSGubo, DSPlane, DSArrow, DSBox, DSScore, DSLife, DSSplash, DSWin, DSLose, DSGround, DSHelp, DSRoad, DSStreet, DSPropeller; /** one per instance of model (if not using instanced-rendering)**/
	std::array<DescriptorSet, 12> DSCity;
	Texture TCity, TArrow, TGround, TScore, TLife, TSplash, TWin, TLose, THelp, TEmit;
	
	// C++ storage for uniform variables
	MetallicUniformBlock uboPlane, uboArrow;
    OpaqueUniformBlock uboBox, uboGround;
    std::array<OpaqueUniformBlock, 12> uboCity;
    EmitUniformBlock uboRoad, uboStreet;
	GlobalUniformBlock gubo;
	OverlayUniformBlock uboScore, uboLife, uboSplash, uboWin, uboLose, uboHelp;
    AnimationUniformBlock uboPropeller;

	GameState gameState = SPLASH;
    glm::vec3 targetPos;
    // moving target random range: values that make it land inside ground
    const int RANGE = 120; // target random position xz range
    const int START = -60; // starting value
    std::vector<glm::vec3> collisionDetectionVertices;

    // city blocks parameters
    const vec3 CITY_STARTING_POS = {-36, 0, -48}; // centers city in the square 120x120 map
    const int CITY_OFFSET = 24; // distance between buildings
    const int CITY_DIM = 3; // in our case 3x4 city so every 3 blocks jump to next row

    const int ROAD_INSTANCES = 6;
    const vec3 ROAD_STARTING_POSITION = {48, 0.15, 48};
    const vec3 ROAD_OFFSET = {- 16.0f, 0, 0};
    const int ROAD_ROWS = 6;

    const int STREET_INSTANCES = 24; // 2 12x1 rows
    const vec3 STREET_STARTING_POSITION = {48, 0.15, - 24};
    const vec3 STREET_OFFSET = {- 8.0f, 0, 24};
    const int STREET_ROWS = 12;

    const float SCORE_OFFSET = 0.15;
    const glm::vec2 SCORE_BOTTOM_LEFT = {-0.9f, 0.8f};
    const float SCORE_WIDTH = 0.10;
    const float LIFE_DISTANCE = -0.2;
    const int WINNING_SCORE = 5; /** or if instances are identical use INSTANCED RENDERING! sharing DS **/
    const int STARTING_LIVES = 3;

    const int PROPELLER_INSTANCES = 2;
    const vec3 PROPELLER_OFFSET = {22.5, 0, 0};

    const vec3 PLANE_STARTING_POS = {48, 0, 0}; // starts in middle of long side offset to the side

    LogarithmicWing wingImplementation = LogarithmicWing(Plane::MAX_WING_LIFT, Plane::MAX_SPEED, Plane::BASE);
    Plane* const plane = new Plane(wingImplementation, collisionDetectionVertices, PLANE_STARTING_POS);
    Package* const box = new Package(plane->getPositionInWorldCoordinates(), plane->getSpeedInWorldCoordinates(), targetPos);
    int score = 0;
    int lives = STARTING_LIVES;

    /**
     * computes the translation vector for a given model among the city models
     * @param index of the model for which to compute the translation
     */
    vec3 computeCityTranslation(int index) {
        return CITY_STARTING_POS + vec3{(index % CITY_DIM) * CITY_OFFSET, 0, (index / CITY_DIM) * CITY_OFFSET};
    }

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Drone delivery";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.06f, 0.4f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 31;
		texturesInPool = 30;
		setsInPool = 31;
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}

    void initGameLogic() {
        gameState = SPLASH;
        targetPos.x = static_cast<float>(rand() % RANGE + START);
        targetPos.y = 0;
        targetPos.z = static_cast<float>(rand() % RANGE + START);
        for (int i = 0; i < MCity.size(); ++i) {
            for (auto v : MCity[i].vertices) {
                collisionDetectionVertices.push_back(
                        v.pos + computeCityTranslation(i));
            }
        }
    }

    /**
     * computes fixed components of ubos only once instead of re-computing them at every frame (e.g. world matrices of fixed objects)
     */
    void initUniforms() {
        uboSplash.mvpMat = glm::mat4(1);
        uboSplash.instancesToDraw = 1.0;

        gubo.DlightDir = glm::normalize(glm::vec3(1, 2, 3));
        gubo.DlightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        gubo.AmbLightColor = glm::vec3(0.9f);

        for (int i = 0; i < MCity.size(); ++i) {
            uboCity[i].amb = 1.0f; uboCity[i].sigma = 1.1;
            uboCity[i].mMat = translate(mat4(1), computeCityTranslation(i));
            uboCity[i].nMat = glm::inverse(glm::transpose(uboCity[i].mMat));
        }

        uboPlane.amb = 1.0f; uboPlane.gamma = 180.0f; uboPlane.sColor = glm::vec3(1.0f);

        uboArrow.amb = 1.0f; uboArrow.gamma = 180.0f; uboArrow.sColor = glm::vec3(1.0f);

        uboBox.amb = 1.0f; uboBox.sigma = 1.1;

        uboRoad.mMat =
                glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(0,1,0))
                * translate(mat4(1), ROAD_STARTING_POSITION);
        uboRoad.amb = 1.0f; uboRoad.sigma = 1.1;
        uboRoad.nMat = glm::inverse(glm::transpose(uboRoad.mMat));
        uboRoad.offset = ROAD_OFFSET;
        uboRoad.dim = static_cast<float>(ROAD_ROWS);

        uboStreet.mMat =
                glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(0,1,0))
                * translate(mat4(1), STREET_STARTING_POSITION);
        uboStreet.amb = 1.0f; uboStreet.sigma = 1.1;
        uboStreet.nMat = glm::inverse(glm::transpose(uboStreet.mMat));
        uboStreet.offset = STREET_OFFSET;
        uboStreet.dim = static_cast<float>(STREET_ROWS);

        /* high gamma makes the ground less shiny and sColor specular reflection color is set to dark green */
        uboGround.amb = 1.0f; uboGround.sigma = 1.1;
        uboGround.mMat = mat4(1);
        uboGround.nMat = glm::inverse(glm::transpose(uboGround.mMat));

        uboScore.mvpMat = mat4(1);
        uboScore.offset = {SCORE_OFFSET, 0}; /** offset between identical instances **/

        uboLife.mvpMat = glm::translate(glm::mat4(1), glm::vec3(0, LIFE_DISTANCE, 0));
        uboLife.offset = {SCORE_OFFSET, 0}; /** offset between identical instances **/

        uboHelp.mvpMat = mat4(1);
        uboHelp.instancesToDraw = 1.0;

        uboWin.mvpMat = mat4(1);
        uboWin.instancesToDraw = 1.0;

        uboLose.mvpMat = mat4(1);
        uboLose.instancesToDraw = 1.0;

        uboPropeller.offset = PROPELLER_OFFSET;
        uboPropeller.time = 0.0;

        cout << "Finished initialising uniforms!\n";
    }
	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLMetallic.init(this, {
					// this array contains the bindings:
					// first  element : the binding number
					// second element : the type of element (buffer or texture)
					//                  using the corresponding Vulkan constant
					// third  element : the pipeline stage where it will be used
					//                  using the corresponding Vulkan constant
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				});

        DSLOpaque.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

        DSLEmit.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT} // for emission texture sampler
        });
				
		DSLOverlay.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				});

        DSLPropeller.init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });

		DSLGubo.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
				});

		// Vertex descriptors
		VClassic.init(this, {
				  // this array contains the bindings
				  // first  element : the binding number
				  // second element : the stride of this binging
				  // third  element : whether this parameter change per vertex or per instance
				  //                  using the corresponding Vulkan constant
				  {0, sizeof(VertexClassic), VK_VERTEX_INPUT_RATE_VERTEX}
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
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexClassic, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexClassic, norm),
				         sizeof(glm::vec3), NORMAL},
				  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexClassic, UV),
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

        VAnimation.init(this, {
                {0, sizeof(VertexAnimation), VK_VERTEX_INPUT_RATE_VERTEX}
                }, {
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexAnimation, pos),
                        sizeof(glm::vec3), POSITION}
                });

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		PMetallic.init(this, &VClassic, "shaders/MetallicVert.spv", "shaders/MetallicFrag.spv", {&DSLGubo, &DSLMetallic});
        PMetallic.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL,
                                      VK_CULL_MODE_BACK_BIT, true);
        // default advanced features: VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false
        POpaque.init(this, &VClassic, "shaders/OpaqueVert.spv", "shaders/OpaqueFrag.spv", {&DSLGubo, &DSLOpaque});
        /** back-face culling cuts groud for all assets with attached ground (park & roller coaster): consider enabling **/
        PEmit.init(this, &VClassic, "shaders/EmitVert.spv", "shaders/EmitFrag.spv", {&DSLGubo, &DSLEmit});
        PEmit.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL,
                                    VK_CULL_MODE_NONE, false); /** ROAD TILES REQUIRE NO BACK-FACE CULLING **/
		POverlay.init(this, &VOverlay, "shaders/OverlayVert.spv", "shaders/OverlayFrag.spv", {&DSLOverlay});
		POverlay.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
 								    VK_CULL_MODE_NONE, true);
        PPropeller.init(this, &VAnimation, "shaders/AnimationVert.spv", "shaders/AnimationFrag.spv", {&DSLPropeller});
        PPropeller.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL,
                                       VK_CULL_MODE_BACK_BIT, true);

		// Models, textures and Descriptors (values assigned to the uniforms)

		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
        for (int i = 0; i < MCity.size(); ++i) {
            std::string modelFile = "Models/city_" + std::to_string(i) + ".mgcg";
            MCity[i].init(this, &VClassic, modelFile, MGCG);
        }

		MPlane.init(this, &VClassic, "Models/plane_001.mgcg", MGCG);
        MArrow.init(this, &VClassic, "Models/tube.obj", OBJ);
        MBox.init(this, &VClassic, "Models/box_005.mgcg", MGCG);
        MRoad.init(this, &VClassic, "Models/road_0.mgcg", MGCG);
        MStreet.init(this, &VClassic, "Models/street_0.mgcg", MGCG);

        // MGround.init(this, &VMesh, "Models/ground.mgcg", MGCG);
        MGround.vertices = {{{-64, 0, -64}, {0, 1, 0}, {0, 0}},
                            {{-64, 0,  64}, {0, 1, 0}, {10, 0}},
                            {{ 64, 0, -64}, {0, 1, 0}, {0, 10}},
                            {{ 64, 0,  64}, {0, 1, 0}, {10, 10}}}; // give UVs values >1 to repeat the texture
        MGround.indices = {0, 1, 2, 1, 3, 2};
        MGround.initMesh(this, &VClassic);

        float scoreHeight = Ar * SCORE_WIDTH; // to make score images square
		MScore.vertices = {{SCORE_BOTTOM_LEFT, {0.0f, 0.0f}}, {{SCORE_BOTTOM_LEFT.x, SCORE_BOTTOM_LEFT.y + scoreHeight}, {0.0f, 1.0f}},
                           {{SCORE_BOTTOM_LEFT.x + SCORE_WIDTH, SCORE_BOTTOM_LEFT.y}, {1.0f, 0.0f}}, {{SCORE_BOTTOM_LEFT.x + SCORE_WIDTH, SCORE_BOTTOM_LEFT.y + scoreHeight}, {1.0f, 1.0f}}};
        MScore.indices = {0, 1, 2, 1, 2, 3};
		MScore.initMesh(this, &VOverlay);

        MLife.vertices = {{SCORE_BOTTOM_LEFT, {0.0f, 0.0f}}, {{SCORE_BOTTOM_LEFT.x, SCORE_BOTTOM_LEFT.y + scoreHeight}, {0.0f, 1.0f}},
                           {{SCORE_BOTTOM_LEFT.x + SCORE_WIDTH, SCORE_BOTTOM_LEFT.y}, {1.0f, 0.0f}}, {{SCORE_BOTTOM_LEFT.x + SCORE_WIDTH, SCORE_BOTTOM_LEFT.y + scoreHeight}, {1.0f, 1.0f}}};
        MLife.indices = {0, 1, 2, 1, 2, 3};
        MLife.initMesh(this, &VOverlay);
		
		// Creates a mesh with direct enumeration of vertices and indices
		MSplash.vertices = {{{-1, -1}, {0, 0}}, {{-1, 1}, {0, 1}},
						 {{ 1,-1}, {1, 0}}, {{ 1, 1}, {1, 1}}};
		MSplash.indices = {0, 1, 2,    1, 2, 3};
		MSplash.initMesh(this, &VOverlay);

        MWin.vertices = MSplash.vertices;
        MWin.indices = MSplash.indices;
        MWin.initMesh(this, &VOverlay);

        MLose.vertices = MSplash.vertices;
        MLose.indices = MSplash.indices;
        MLose.initMesh(this, &VOverlay);

        MHelp.vertices = MSplash.vertices;
        MHelp.indices = MSplash.indices;
        MHelp.initMesh(this, &VOverlay);

        MPropeller.init(this, &VAnimation, "Models/propeller_animation.obj", OBJ);
		
		// Create the textures
		// The second parameter is the file name
		TCity.init(this, "textures/Textures_City.png");
        TArrow.init(this, "textures/tube.png");
        TGround.init(this, "textures/grass.jpg");
		TScore.init(this, "textures/BoxScore.jpg");
        TLife.init(this, "textures/life.png");
		TSplash.init(this, "textures/splash.png");
        TWin.init(this, "textures/win.png");
        TLose.init(this, "textures/lose.png");
        THelp.init(this, "textures/help.png");
        TEmit.init(this, "textures/city_emit.png");

		initGameLogic();
        initUniforms();
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		PMetallic.create();
        POpaque.create();
        PEmit.create();
		POverlay.create();
        PPropeller.create();

        for (auto &dsCity : DSCity) {
            dsCity.init(this, &DSLOpaque, {
                {0, UNIFORM, sizeof(OpaqueUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}});
        }

		DSPlane.init(this, &DSLMetallic, {
					{0, UNIFORM, sizeof(MetallicUniformBlock), nullptr},
					{1, TEXTURE, 0,                            &TCity}
				});
        DSArrow.init(this, &DSLMetallic, {
                {0, UNIFORM, sizeof(MetallicUniformBlock), nullptr},
                {1, TEXTURE, 0,                            &TArrow}
        });
        DSBox.init(this, &DSLOpaque, {
                {0, UNIFORM, sizeof(OpaqueUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity}
        });
        DSRoad.init(this, &DSLEmit, {
                {0, UNIFORM, sizeof(OpaqueUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity},
                {2, TEXTURE, 0, &TEmit}
        });
        DSStreet.init(this, &DSLEmit, {
                {0, UNIFORM, sizeof(OpaqueUniformBlock), nullptr},
                {1, TEXTURE, 0, &TCity},
                {2, TEXTURE, 0, &TEmit}
        });
        DSGround.init(this, &DSLOpaque, {
                {0, UNIFORM, sizeof(OpaqueUniformBlock), nullptr},
                {1, TEXTURE, 0, &TGround}
        });
		DSScore.init(this, &DSLOverlay, {
					{0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
					{1, TEXTURE, 0, &TScore}
				});
        DSLife.init(this, &DSLOverlay, {
                {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                {1, TEXTURE, 0, &TLife}
        });
		DSSplash.init(this, &DSLOverlay, {
					{0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
					{1, TEXTURE, 0, &TSplash}
				});
        DSWin.init(this, &DSLOverlay, {
                {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                {1, TEXTURE, 0, &TWin}
        });
        DSLose.init(this, &DSLOverlay, {
                {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                {1, TEXTURE, 0, &TLose}
        });
        DSHelp.init(this, &DSLOverlay, {
                {0, UNIFORM, sizeof(OverlayUniformBlock), nullptr},
                {1, TEXTURE, 0, &THelp}
        });
        DSPropeller.init(this, &DSLPropeller, {
                {0, UNIFORM, sizeof(AnimationUniformBlock), nullptr}
        });
		DSGubo.init(this, &DSLGubo, {
					{0, UNIFORM, sizeof(GlobalUniformBlock), nullptr}
				});
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		PMetallic.cleanup();
        POpaque.cleanup();
        PEmit.cleanup();
		POverlay.cleanup();
        PPropeller.cleanup();

		// Cleanup datasets
        for (auto &dsCity : DSCity) {
            dsCity.cleanup();
        }

		DSPlane.cleanup();
        DSBox.cleanup();
        DSRoad.cleanup();
        DSStreet.cleanup();
        DSArrow.cleanup();
        DSGround.cleanup();
		DSScore.cleanup();
        DSLife.cleanup();
		DSSplash.cleanup();
        DSWin.cleanup();
        DSLose.cleanup();
		DSGubo.cleanup();
        DSHelp.cleanup();
        DSPropeller.cleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {
		// Cleanup textures
		TCity.cleanup();
        TArrow.cleanup();
        TGround.cleanup();
		TScore.cleanup();
        TLife.cleanup();
		TSplash.cleanup();
        TWin.cleanup();
        TLose.cleanup();
        THelp.cleanup();
        TEmit.cleanup();
		
		// Cleanup models
        for (auto &mCity : MCity) {
            mCity.cleanup();
        }

		MPlane.cleanup();
        MBox.cleanup();
        MRoad.cleanup();
        MStreet.cleanup();
        MArrow.cleanup();
        MGround.cleanup();
		MScore.cleanup();
        MLife.cleanup();
		MSplash.cleanup();
        MWin.cleanup();
        MLose.cleanup();
        MHelp.cleanup();
        MPropeller.cleanup();
		
		// Cleanup descriptor set layouts
		DSLMetallic.cleanup();
        DSLOpaque.cleanup();
        DSLEmit.cleanup();
		DSLOverlay.cleanup();
        DSLPropeller.cleanup();

		DSLGubo.cleanup();
		
		// Destroys the pipelines
		PMetallic.destroy();
        POpaque.destroy();
        PEmit.destroy();
		POverlay.destroy();
        PPropeller.destroy();
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	/**
	 * CAREFUL: ORDER OF CALLS MATTERS!
	 * for each pipeline you have to gubo.bind(pipeline1), pipeline1.bind(), model1.bind(), ds1.bind(pipeline1), model2.bind(), ds2.bind(pipeline1)...
	 * without mixing pipeline order (e.g. WRONG gubo.bind(pipeline1), gubo.bind(pipeline2), pipeline1.bind(), pipeline2.bind())
	 */
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		// sets global uniforms (see below fro parameters explanation)
		DSGubo.bind(commandBuffer, PMetallic, 0, currentImage);

        PMetallic.bind(commandBuffer);

        MPlane.bind(commandBuffer);
        DSPlane.bind(commandBuffer, PMetallic, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPlane.indices.size()), 1, 0, 0, 0);

        MArrow.bind(commandBuffer);
        DSArrow.bind(commandBuffer, PMetallic, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MArrow.indices.size()), 1, 0, 0, 0);

        PPropeller.bind(commandBuffer);
        MPropeller.bind(commandBuffer);
        DSPropeller.bind(commandBuffer, PPropeller, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MPropeller.indices.size()), PROPELLER_INSTANCES, 0, 0, 0);

        DSGubo.bind(commandBuffer, POpaque, 0, currentImage);

		// binds the pipeline
        POpaque.bind(commandBuffer);
		// For a pipeline object, this command binds the corresponing pipeline to the command buffer passed in its parameter

		// binds the model
        for (int i = 0; i < MCity.size(); ++i) {
            MCity[i].bind(commandBuffer);
            DSCity[i].bind(commandBuffer, POpaque, 1, currentImage);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(MCity[i].indices.size()), 1, 0, 0, 0);
        }

        MBox.bind(commandBuffer);
        DSBox.bind(commandBuffer, POpaque, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MBox.indices.size()), 1, 0, 0, 0);

        MGround.bind(commandBuffer);
        DSGround.bind(commandBuffer, POpaque, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MGround.indices.size()), 1, 0, 0, 0);

        DSGubo.bind(commandBuffer, PEmit, 0, currentImage);

        PEmit.bind(commandBuffer);

        MRoad.bind(commandBuffer);
        DSRoad.bind(commandBuffer, PEmit, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MRoad.indices.size()), ROAD_INSTANCES, 0, 0, 0);

        MStreet.bind(commandBuffer);
        DSStreet.bind(commandBuffer, PEmit, 1, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MStreet.indices.size()), STREET_INSTANCES, 0, 0, 0);

		POverlay.bind(commandBuffer);
		MScore.bind(commandBuffer);
		DSScore.bind(commandBuffer, POverlay, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MScore.indices.size()), WINNING_SCORE, 0, 0, 0);
        DSLife.bind(commandBuffer, POverlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MLife.indices.size()), STARTING_LIVES, 0, 0, 0);
		MSplash.bind(commandBuffer);
		DSSplash.bind(commandBuffer, POverlay, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MSplash.indices.size()), 1, 0, 0, 0);
        MWin.bind(commandBuffer);
        DSWin.bind(commandBuffer, POverlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MWin.indices.size()), 1, 0, 0, 0);
        MLose.bind(commandBuffer);
        DSLose.bind(commandBuffer, POverlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MLose.indices.size()), 1, 0, 0, 0);
        MHelp.bind(commandBuffer);
        DSHelp.bind(commandBuffer, POverlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(MHelp.indices.size()), 1, 0, 0, 0);
	}

    void updateSplashUniformBuffer(uint32_t currentImage, UserInputs& userInputs) {
        uboSplash.visible = (gameState == SPLASH) ? 1.0f : 0.0f;
        DSSplash.map(currentImage, &uboSplash, sizeof(uboSplash), 0);
    }

    void updatePlayingUniformBuffer(uint32_t currentImage, UserInputs& userInputs) {
        /**
         * keep gubo.eyePos fixed
         * compute the position of the plane, i.e. the plane's world matrix + view and projection matrices based on plane position (world matrix)
         * terrain world matrix stays fixed (as already computed) and vp matrices are same as vp of plane
         *
         * ==> only thing that needs computing is WVP matrix of the plane, all others act accordingly
         */

        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.f;

        /**
         * MPark[0].vertices access map vertices for collision detection: finding top 3 closest vertices to player not enough
         * because you can't know if the condition to enforce is player.xyz >< terrain.xyz,
         * but you can find the vertex "terrain" with closest xz and enforce that player.y > terrain.y
         */

        if(plane->isCollisionDetected() && gameState == PLAYING) {
            lives--;
        }

        glm::mat4 planeWorldMat = plane->computeWorldMatrix();
        glm::vec3 camPos = computeCameraPosition(planeWorldMat, userInputs);
        glm::vec3 planePos = plane->getPositionInWorldCoordinates();
        glm::mat4 viewMat = glm::lookAt(camPos, planePos, glm::vec3(0.0f, 1.0f, 0.0f)) ;
        glm::mat4 projMat = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        projMat[1][1] *= -1;

        gubo.eyePos = camPos;
        gubo.usePointLight = (userInputs.handleQ)? 1.0 : 0.0;
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

        for (int i = 0; i < MCity.size(); ++i) {
            uboCity[i].mvpMat = projMat * viewMat * uboCity[i].mMat;
            DSCity[i].map(currentImage, &uboCity[i], sizeof(uboCity[i]), 0);
        }

        uboPlane.mvpMat = projMat * viewMat * planeWorldMat;
        uboPlane.mMat = planeWorldMat; // plane world mat changes at each frame
        uboPlane.nMat = glm::inverse(glm::transpose(planeWorldMat));
        DSPlane.map(currentImage, &uboPlane, sizeof(uboPlane), 0);

        if (box->isTargetHit() && gameState == PLAYING) {
            targetPos.x = static_cast<float>(rand() % RANGE + START);
            targetPos.z = static_cast<float>(rand() % RANGE + START);
            if (gameState == 1) score++;
        }

        uboArrow.mMat = glm::translate(glm::mat4(1), glm::vec3(targetPos.x, -2, targetPos.z));
        uboArrow.mvpMat = projMat * viewMat * uboArrow.mMat;
        uboArrow.nMat = glm::inverse(glm::transpose(uboArrow.mMat));
        DSArrow.map(currentImage, &uboArrow, sizeof(uboArrow), 0);

        uboBox.mMat = box->computeWorldMatrix();
        uboBox.mvpMat = projMat * viewMat * uboBox.mMat;
        uboBox.nMat = glm::inverse(glm::transpose(uboBox.mMat));
        DSBox.map(currentImage, &uboBox, sizeof(uboBox), 0);

        uboRoad.mvpMat = projMat * viewMat * uboRoad.mMat;
        DSRoad.map(currentImage, &uboRoad, sizeof(uboRoad), 0);

        uboStreet.mvpMat = projMat * viewMat * uboStreet.mMat;
        DSStreet.map(currentImage, &uboStreet, sizeof(uboStreet), 0);

        uboGround.mvpMat = projMat * viewMat * uboGround.mMat;
        DSGround.map(currentImage, &uboGround, sizeof(uboGround), 0);

        uboScore.visible = (gameState == 1) ? 1.0f : 0.0f;
        uboScore.instancesToDraw = static_cast<float>(WINNING_SCORE - score);
        DSScore.map(currentImage, &uboScore, sizeof(uboScore), 0);

        uboLife.visible = (gameState == 1) ? 1.0f : 0.0f;
        uboLife.instancesToDraw = static_cast<float>(lives);
        DSLife.map(currentImage, &uboLife, sizeof(uboLife), 0);

        uboHelp.visible = (gameState == 1) ? 1.0f : 0.0f;
        DSHelp.map(currentImage, &uboHelp, sizeof(uboHelp), 0);

        //cout << "plane x speed: " << plane->getSpeedInPlaneCoordinates().x << "\n";
        uboPropeller.mvpMat = projMat * viewMat * translate(scale(planeWorldMat, vec3(0.2)), {- PROPELLER_OFFSET.x / 2.0, 5.0, 7.0});
        uboPropeller.visible = glm::length(plane->getSpeedInWorldCoordinates()) > 0.01; // propeller animation visible only if plane moving forward
        uboPropeller.time += userInputs.deltaT;
        DSPropeller.map(currentImage, &uboPropeller, sizeof(uboPropeller), 0);
    }

    void updateWinUniformBuffer(uint32_t currentImage, UserInputs& userInputs) {
        uboWin.visible = (gameState == WON) ? 1.0f : 0.0f;
        DSWin.map(currentImage, &uboWin, sizeof(uboWin), 0);
    }

    void updateLoseUniformBuffer(uint32_t currentImage, UserInputs& userInputs) {
        uboLose.visible = (gameState == LOST) ? 1.0f : 0.0f;
        DSLose.map(currentImage, &uboLose, sizeof(uboLose), 0);
    }

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		// Standard procedure to quit when the ESC key is pressed
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

        auto* userInputs = new UserInputs(this, gameState);
        plane->updateInputs(userInputs);
        box->updateInputs(userInputs);

		switch(gameState) {
		  case SPLASH: {
              plane->resetState();
              if(userInputs->handleNext) gameState = PLAYING;
              break;
          }
		  case PLAYING: {
              if (lives <= 0) {
                  gameState = LOST;
                  break;
              }
              if (score >= WINNING_SCORE) gameState = WON;
              break;
          }
          case WON:
          case LOST:
          {
              score = 0;
              lives = STARTING_LIVES;
              plane->resetState();
              if(userInputs->handleNext) gameState = SPLASH;
              break;
          }
		}

        updateSplashUniformBuffer(currentImage, *userInputs);
        updatePlayingUniformBuffer(currentImage, *userInputs);
        updateWinUniformBuffer(currentImage, *userInputs);
        updateLoseUniformBuffer(currentImage, *userInputs);
	}

    /**
    * @param world world transform matrix
    * @param camDistance distance from tracked object in object's coordinates
    * @param camHeight height from tracked object in object's coordinates
    * @param camPitch pitch in radians
    * @return camera position in world coordinates implementing damping
    */
    vec3 computeCameraPosition(const mat4& world, UserInputs& userInputs) {
        const float camHeight = 0.25;
        const float camDistance = 25.0;
        const float camPitch = 0.5;

        static auto posDamper = Damper<vec3>(10);
        if (userInputs.handleR) return posDamper.damp({PLANE_STARTING_POS.x, 3, PLANE_STARTING_POS.z}, userInputs.deltaT);

        vec3 posNew =
                world
                * glm::rotate(glm::mat4(1.0f), glm::radians(- 90.0f), glm::vec3(0,1,0))
                * glm::vec4(- camDistance * std::cos(camPitch),
                          camHeight + camDistance * std::sin(camPitch),
                          0.0f,
                          1);
        posNew.y = std::max(posNew.y, 0.5f); // avoids camera from going below the ground level

        return posDamper.damp(posNew, userInputs.deltaT);
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