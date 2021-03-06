//==============================================================================
/*
    \author    Your Name
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
#include "MyProxyAlgorithm.h"
#include "MyMaterial.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

MyMaterialPtr toolMaterial;

const std::string textureFiles[9] = 
{
    "Organic_Scales_001_colour.jpg", 
    "redbricks-colour.png", 
    "Fabric_002_colour.jpg",
    "bumps.png", 
    "Metal_plate_001_colour.jpg", 
    "friction.jpg",
    "Tiles_001_colour.jpg", 
    "bark-colour.png", 
    "Cork_001_colour.jpg"
};

const std::string heightFiles[9] = 
{
    "Organic_Scales_001_height.jpg", 
    "redbricks-height.png", 
    "Fabric_002_height.jpg",
    "bumps.png", 
    "Metal_plate_001_height.jpg", 
    "friction.jpg",
    "Tiles_001_height.jpg", 
    "bark-height.png", 
    "Cork_001_height.jpg"
};

const std::string normalFiles[9] = 
{
    "Organic_Scales_001_normal.jpg", 
    "redbricks-normal.png", 
    "Fabric_002_normal.jpg",
    "bumps.png", 
    "Metal_plate_001_normal.jpg", 
    "friction.jpg",
    "Tiles_001_normal.jpg", 
    "bark-normal.png", 
    "Cork_001_normal.jpg"
};

const std::string roughnessFiles[9] = 
{
    "Organic_scales_001_roughness.jpg", 
    "redbricks-roughness.png", 
    "Fabric_002_roughness.jpg",
    "bumps.png", 
    "Metal_Plate_001_roughness.jpg", 
    "friction.jpg",
    "Tiles_001_roughness.jpg", 
    "bark-roughness.png", 
    "Cork_001_roughness.jpg"
};

std::vector<cTexture2dPtr> albedoTextures;
std::vector<cTexture2dPtr> heightTextures;
std::vector<cTexture2dPtr> normalTextures;
std::vector<cTexture2dPtr> roughnessTextures;

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cSpotLight* light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a label to display the rates [Hz] at which the simulation is running
cLabel* labelRates;
cLabel* labelDebug;

// a small sphere (cursor) representing the haptic device
cToolCursor* tool;

// a pointer to the custom proxy rendering algorithm inside the tool
MyProxyAlgorithm* proxyAlgorithm;

// nine objects with different surface textures that we want to render
cMultiMesh *objects[3][3];

// flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// flag to indicate if the haptic simulation has terminated
bool simulationFinished = false;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width  = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;

int texIndex = -1;

cVector3d workspaceOffset(0.0, 0.0, 0.0);
cVector3d cameraLookAt(0.0, 0.0, 0.0);

//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);

void CreateTextures()
{
    for (int i = 0; i < 9; i++)
    {
        cTexture2dPtr albedoMap = cTexture2d::create();
        albedoMap->loadFromFile("images/" + textureFiles[i]);
        albedoMap->setWrapModeS(GL_REPEAT);
        albedoMap->setWrapModeT(GL_REPEAT);
        albedoMap->setUseMipmaps(true);

        // create a height texture map for this mesh object
        cTexture2dPtr heightMap = cTexture2d::create();
        heightMap->loadFromFile("images/" + heightFiles[i]);
        heightMap->setWrapModeS(GL_REPEAT);
        heightMap->setWrapModeT(GL_REPEAT);
        heightMap->setUseMipmaps(true);

        // create a normal texture map for this mesh object
        cTexture2dPtr normalMap = cTexture2d::create();
        normalMap->loadFromFile("images/" + normalFiles[i]);
        normalMap->setWrapModeS(GL_REPEAT);
        normalMap->setWrapModeT(GL_REPEAT);
        normalMap->setUseMipmaps(true);

        // create a roughness texture map for this mesh object
        cTexture2dPtr roughnessMap = cTexture2d::create();
        roughnessMap->loadFromFile("images/" + roughnessFiles[i]);
        roughnessMap->setWrapModeS(GL_REPEAT);
        roughnessMap->setWrapModeT(GL_REPEAT);
        roughnessMap->setUseMipmaps(true);

        albedoTextures.push_back(albedoMap);
        heightTextures.push_back(heightMap);
        normalTextures.push_back(normalMap);
        roughnessTextures.push_back(roughnessMap);
    }
}

void ChangeToolTexture()
{
    MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(tool->m_hapticPoint->m_sphereProxy->m_material);
    material = MyMaterial::create();
    material->setUseHapticShading(true);
    material->Type = texIndex;

    // assign textures to the mesh
    tool->m_hapticPoint->m_sphereProxy->m_texture = albedoTextures[texIndex];
    material->m_height_map = heightTextures[texIndex];
    material->m_normal_map = normalTextures[texIndex];
    material->m_roughness_map = roughnessTextures[texIndex];
    tool->m_hapticPoint->m_sphereProxy->setUseTexture(true);

    tool->setRadius(0.01, 0.01);

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            cMultiMesh* object = objects[i][j];
            object->createAABBCollisionDetector(0.01);
        }
    }
    toolMaterial = material;
}

//==============================================================================
/*
    TEMPLATE:    application.cpp

    Description of your application.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;

    cout << "[+] (On Falcon) - Apply next texture to tool" << endl;
    cout << "[-] (On Falcon) - Apply previous texture to tool" << endl;
    cout << "[swirl] (On Falcon) - Disable texture on tool" << endl;

    cout << "[q] - Exit application" << endl;
    cout << endl << endl;


    //--------------------------------------------------------------------------
    // OPENGL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "Force Shading & Haptic Textures (Teresa Van)", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setBlack();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set( cVector3d (0.1, 0.0, 0.07),    // camera position (eye)
                 cameraLookAt,    // look at position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 1.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.01);
    camera->setStereoFocalLength(0.5);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a directional light source
    light = new cSpotLight(world);

    // insert light source inside world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // position the light source
    light->setLocalPos(0.7, 0.3, 1.0);

    // define the direction of the light beam
    light->setDir(-0.5,-0.2,-0.8);

    // enable this light source to generate shadows
    light->setShadowMapEnabled(true);

    // set the resolution of the shadow map
    light->m_shadowMap->setQualityHigh();

    // set light cone half angle
    light->setCutOffAngleDeg(10);

    // use a point avatar for this scene
    double toolRadius = 0.0;

    //--------------------------------------------------------------------------
    // [CPSC.86] TEXTURED OBJECTS
    //--------------------------------------------------------------------------

    CreateTextures();

    const double objectSpacing = 0.09;

    int count = 0;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            objects[i][j] = new cMultiMesh();
            cMultiMesh* object = objects[i][j];

            // load geometry from file and compute additional properties
            object->loadFromFile("tray.obj");
            object->createAABBCollisionDetector(toolRadius);
            object->computeBTN();

            // object->setWireMode(true);

            // obtain the first (and only) mesh from the object
            cMesh* mesh = object->getMesh(0);

            // replace the object's material with a custom one
            mesh->m_material = MyMaterial::create();
            mesh->m_material->setWhite();
            mesh->m_material->setUseHapticShading(true);
            object->setStiffness(2000.0, true);

            MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
            material->Type = count;

            // assign textures to the mesh
            mesh->m_texture = albedoTextures[count];
            material->m_height_map = heightTextures[count];
            material->m_normal_map = normalTextures[count];
            material->m_roughness_map = roughnessTextures[count];
            mesh->setUseTexture(true);

            // set the position of this object
            double xpos = -objectSpacing + i * objectSpacing;
            double ypos = -objectSpacing + j * objectSpacing;
            object->setLocalPos(xpos, ypos);

            world->addChild(object);

            count++;
        }
    }

    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get a handle to the first haptic device
    handler->getDevice(hapticDevice, 0);

    // if the device has a gripper, enable the gripper to simulate a user switch
    hapticDevice->setEnableGripperUserSwitch(true);

    tool = new cToolCursor(world);
    world->addChild(tool);

    // [CPSC.86] replace the tool's proxy rendering algorithm with our own
    proxyAlgorithm = new MyProxyAlgorithm;
    delete tool->m_hapticPoint->m_algorithmFingerProxy;
    tool->m_hapticPoint->m_algorithmFingerProxy = proxyAlgorithm;

    tool->m_hapticPoint->m_sphereProxy->m_material->setWhite();
    // tool->m_hapticPoint->m_sphereProxy->setWireMode(true);
    
    tool->setRadius(0.001, toolRadius);

    tool->setHapticDevice(hapticDevice);

    tool->setWaitForSmallForce(true);

    tool->start();

    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    cFontPtr font = NEW_CFONTCALIBRI20();

    // create a label to display the haptic and graphic rates of the simulation
    labelRates = new cLabel(font);
    labelRates->m_fontColor.setWhite();
    camera->m_frontLayer->addChild(labelRates);

    // create a label to display the haptic and graphic rates of the simulation
    labelDebug = new cLabel(font);
    labelDebug->m_fontColor.setWhite();
    camera->m_frontLayer->addChild(labelDebug);

    camera->setWireMode(true);
    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        // render graphics
        updateGraphics();

        // swap buffers
        glfwSwapBuffers(window);

        // process events
        glfwPollEvents();

        // signal frequency counter
        freqCounterGraphics.signal(1);
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width  = a_width;
    height = a_height;
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if (a_action != GLFW_PRESS)
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }
    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    hapticDevice->close();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // update haptic and graphic rate data
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);


    labelDebug->setText(cStr(proxyAlgorithm->m_debugValue) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.x()) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.y()) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.z())
                    );

    // update position of label
    labelDebug->setLocalPos((int)(0.5 * (width - labelDebug->getWidth())), 40);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////
    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(width, height);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------
void updateWorkspace(cVector3d devicePosition)
{
    if (devicePosition.x() < -0.025)
        workspaceOffset = cVector3d(-0.000075, 0.0, 0.0);
    else if (devicePosition.x() > 0.035)
        workspaceOffset = cVector3d(0.000075, 0.0, 0.0);
    else if (devicePosition.y() < -0.035)
        workspaceOffset = cVector3d(0.0, -0.000075, 0.0);
    else if (devicePosition.y() > 0.035)
        workspaceOffset = cVector3d(0.0, 0.000075, 0.0);
    else
        workspaceOffset = cVector3d(0.0, 0.0, 0.0);

    cVector3d toolPosition = tool->getLocalPos();
    cameraLookAt += workspaceOffset;
    toolPosition += workspaceOffset;

    tool->setLocalPos(toolPosition);
    camera->set( camera->getLocalPos() + workspaceOffset,    // camera position (eye)
                cameraLookAt,    // look at position (target)
                cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector
}

void updateHaptics(void)
{
    // simulation in now running
    simulationRunning  = true;
    simulationFinished = false;

    bool leftPressed = false;
    bool rightPressed = false;
    bool midPressed = false;

    // main haptic simulation loop
    while(simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////
        // READ HAPTIC DEVICE
        /////////////////////////////////////////////////////////////////////

        // read position
        cVector3d position;
        hapticDevice->getPosition(position);

        // read orientation
        cMatrix3d rotation;
        hapticDevice->getRotation(rotation);

        // read user-switch status (button 0)
        bool middle = false;
        bool left = false;
        bool right = false;

        hapticDevice->getUserSwitch(0, middle);
        hapticDevice->getUserSwitch(1, left);
        hapticDevice->getUserSwitch(3, right);
        
        if (middle) midPressed = true;
        else if (left) leftPressed = true;
        else if (right) rightPressed = true;

        if (midPressed && !middle)
        {
            midPressed = false;

            tool->m_hapticPoint->m_sphereProxy->setUseTexture(false);
            tool->setRadius(0.001, 0.0);

        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                cMultiMesh* object = objects[i][j];
                object->createAABBCollisionDetector(0.0);
            }
        }
            toolMaterial = nullptr;
        }
        else if (leftPressed && !left)
        {
            texIndex--;
            texIndex = ((texIndex % 9) + 9) % 9;
            ChangeToolTexture();
            leftPressed = false;
        }
        else if (rightPressed && !right)
        {
            texIndex++;
            texIndex = texIndex % 9;
            ChangeToolTexture();
            rightPressed = false;
        }

        world->computeGlobalPositions();

        /////////////////////////////////////////////////////////////////////
        // UPDATE 3D CURSOR MODEL
        /////////////////////////////////////////////////////////////////////

        tool->updateFromDevice();

        /////////////////////////////////////////////////////////////////////
        // COMPUTE FORCES
        /////////////////////////////////////////////////////////////////////

        tool->computeInteractionForces();

        cVector3d force(0, 0, 0);
        cVector3d torque(0, 0, 0);
        double gripperForce = 0.0;


        /////////////////////////////////////////////////////////////////////
        // APPLY FORCES
        /////////////////////////////////////////////////////////////////////

        tool->applyToDevice();
        updateWorkspace(position);

        // signal frequency counter
        freqCounterHaptics.signal(1);
    }

    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------
