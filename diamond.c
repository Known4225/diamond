/* TODO
- Make table of all colors and allow user to edit, merge, and assign letters to them
*/

// #define TURTLE_IMPLEMENTATION
#include "stb_image_write.h"
#include "turtle.h"
#include <time.h>

int32_t import(char *filename);

typedef enum {
    RESIZE_MODE_LINEAR = 0,
    RESIZE_MODE_SRGB = 1,
    RESIZE_MODE_NEAREST_NEIGHBOR = 2,
} resize_mode_t;

typedef enum {
    DIAMOND_KEY_LMB = 0,
    DIAMOND_KEY_RMB = 0,
    DIAMOND_KEY_LEFT_ARROW = 2,
    DIAMOND_KEY_RIGHT_ARROW = 3,
} diamond_key_t;

typedef enum {
    DIAMOND_UI_MODE_IMAGE = 0,
    DIAMOND_UI_MODE_COLOR = 1,
} diamond_ui_mode_t;

typedef struct {
    /* original file */
    char originalFilename[4096];
    int32_t originalWidth;
    int32_t originalHeight;
    int32_t originalChannels;
    uint8_t *originalData;
    turtle_texture_t originalTexture;
    /* transform */
    int32_t resizeMode;
    tt_dropdown_t *resizeModeDropdown;
    /* diamond file */
    uint8_t *diamondFilename[4096];
    int32_t diamondWidth;
    int32_t diamondHeight;
    int32_t diamondChannels;
    uint8_t *diamondData;
    turtle_texture_t diamondTexture;
    /* UI */
    tt_slider_t *resolutionSlider;
    tt_button_t *imageButton;
    tt_button_t *colorButton;
    diamond_ui_mode_t mode;
    int8_t keys[10];
} diamond_t;

diamond_t self;

void init() {
    /* original file */
    self.originalFilename[0] = '\0';
    self.originalData = NULL;
    self.originalTexture = -1;
    /* transform */
    self.resizeMode = RESIZE_MODE_LINEAR;
    list_t *resizeModeOptions = list_init();
    list_append(resizeModeOptions, (unitype) "Linear", 's');
    list_append(resizeModeOptions, (unitype) "SRGB", 's');
    list_append(resizeModeOptions, (unitype) "Nearest Neighbor", 's');
    self.resizeModeDropdown = dropdownInit("Resize Mode", resizeModeOptions, &self.resizeMode, TT_DROPDOWN_ALIGN_CENTER, 220, 20, 8);
    self.resizeModeDropdown -> color[TT_COLOR_SLOT_DROPDOWN_TEXT] = TT_COLOR_COMPONENT_ALTERNATE;
    /* diamond file */
    self.diamondData = NULL;
    self.diamondTexture = -1;
    /* UI */
    self.mode = DIAMOND_UI_MODE_IMAGE;
    self.resolutionSlider = sliderInit("Resolution", NULL, TT_SLIDER_TYPE_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, 220, 60, 8, 60, 1, 200, 1);
    self.resolutionSlider -> color[TT_COLOR_SLOT_SLIDER_TEXT] = TT_COLOR_COMPONENT_ALTERNATE;
    self.resolutionSlider -> scale = TT_SLIDER_SCALE_EXP;
    self.resolutionSlider -> defaultValue = 45;
    self.resolutionSlider -> value = 45;
    self.imageButton = buttonInit("Image", NULL, 320, -172, 10);
    self.imageButton -> align = TT_BUTTON_ALIGN_RIGHT;
    self.imageButton -> color[TT_COLOR_SLOT_BUTTON_CLICKED] = TT_COLOR_BACKGROUND_COMPLEMENT;
    self.colorButton = buttonInit("Color", NULL, 280, -172, 10);
    self.colorButton -> align = TT_BUTTON_ALIGN_RIGHT;
    self.colorButton -> color[TT_COLOR_SLOT_BUTTON_CLICKED] = TT_COLOR_BACKGROUND_COMPLEMENT;

    /* default file */
    import("images/thumbnail.png");
}

/* import image */
int32_t import(char *filename) {
    /* load image */
    strcpy(self.originalFilename, filename);
    self.originalData = stbi_load(filename, &self.originalWidth, &self.originalHeight, &self.originalChannels, 3);
    if (self.originalData == NULL) {
        printf("Failed to load image from %s\n", filename);
        return 1;
    }
    printf("Successfully loaded image from %s\n", filename);
    printf("- width: %d\n", self.originalWidth);
    printf("- height: %d\n", self.originalHeight);
    printf("- channels: %d\n", self.originalChannels);
    self.originalChannels = 3;
    /* create texture */
    self.originalTexture = turtleTextureLoadArray(self.originalData, self.originalWidth, self.originalHeight, GL_RGB);
    return 0;
}

unsigned char *resize_nearest_neighbor( const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes, unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes, stbir_pixel_layout pixel_type) {
    int32_t channels = input_stride_in_bytes / input_w;
    uint8_t *output = malloc(output_w * output_h * channels);
    if (output == NULL) {
        return NULL;
    }
    double xScale = (double) input_w / output_w;
    double yScale = (double) input_h / output_h;
    for (int32_t i = 0; i < output_h; i++) {
        for (int32_t j = 0; j < output_w; j++) {
            for (int32_t k = 0; k < channels; k++) {
                int32_t scratchIndex = (((int32_t) (i * yScale)) * input_w + ((int32_t) (j * xScale))) * channels + k;
                // printf("image[%d] = scratch[%d] = %d\n", i * desiredWidth * channels + j * channels + k, scratchIndex, scratch[scratchIndex]);
                output[(i * output_w + j) * channels + k] = input_pixels[scratchIndex]; // nearest neighbor (top left neighbor)
            }
        }
    }
    return output;
}

void transform() {
    if (self.diamondTexture != -1) {
        turtleTextureUnload(self.diamondTexture);
    }
    self.diamondTexture = -1;
    if (self.originalData == NULL) {
        return;
    }
    self.diamondWidth = (double) self.originalWidth / self.originalHeight * self.resolutionSlider -> value;
    self.diamondHeight = self.resolutionSlider -> value;
    self.diamondChannels = 3;
    if (self.diamondData != NULL) {
        free(self.diamondData);
    }
    if (self.resizeMode == RESIZE_MODE_LINEAR) {
        self.diamondData = stbir_resize_uint8_linear(self.originalData, self.originalWidth, self.originalHeight, self.originalChannels * self.originalWidth, NULL, self.diamondWidth, self.diamondHeight, self.diamondChannels * self.diamondWidth, STBIR_RGB);
        if (self.diamondData == NULL) {
            printf("resized linear failed\n");
            return;
        }
    } else if (self.resizeMode == RESIZE_MODE_SRGB) {
        self.diamondData = stbir_resize_uint8_srgb(self.originalData, self.originalWidth, self.originalHeight, self.originalChannels * self.originalWidth, NULL, self.diamondWidth, self.diamondHeight, self.diamondChannels * self.diamondWidth, STBIR_RGB);
        if (self.diamondData == NULL) {
            printf("resized srgb failed\n");
            return;
        }
    } else if (self.resizeMode == RESIZE_MODE_NEAREST_NEIGHBOR) {
        self.diamondData = resize_nearest_neighbor(self.originalData, self.originalWidth, self.originalHeight, self.originalChannels * self.originalWidth, NULL, self.diamondWidth, self.diamondHeight, self.diamondChannels * self.diamondWidth, STBIR_RGB);
        if (self.diamondData == NULL) {
            printf("resized nearest neighbor failed\n");
            return;
        }
    }
    self.diamondTexture = turtleTextureLoadArray(self.diamondData, self.diamondWidth, self.diamondHeight, GL_RGB);
}

void renderDotImage(double x, double y, double height, int8_t renderDimensions, int8_t circle) {
    /* render transformed image */
    if (self.diamondTexture != -1) {
        double diamondAspect = (double) self.diamondWidth / self.diamondHeight;
        double diamondXLeft = x - height / 2 * diamondAspect;
        double diamondXRight = x + height / 2 * diamondAspect;
        double diamondY = y - height / 2;
        /* render diamond art */
        double circleY = diamondY + height - height / self.diamondHeight / 2;
        double radius = height / 2 / self.diamondHeight;
        for (int32_t i = 0; i < self.diamondHeight; i++) {
            double circleX = diamondXLeft + height * diamondAspect / self.diamondWidth / 2;
            for (int32_t j = 0; j < self.diamondWidth; j++) {
                if (circle) {
                    /* render circles */
                    turtleCircleColor(circleX, circleY, radius, self.diamondData[i * self.diamondWidth * 3 + j * 3], self.diamondData[i * self.diamondWidth * 3 + j * 3 + 1], self.diamondData[i * self.diamondWidth * 3 + j * 3 + 2], 255);
                } else {
                    /* render squares */
                    turtleRectangleColor(circleX - radius, circleY - radius, circleX + radius, circleY + radius, self.diamondData[i * self.diamondWidth * 3 + j * 3], self.diamondData[i * self.diamondWidth * 3 + j * 3 + 1], self.diamondData[i * self.diamondWidth * 3 + j * 3 + 2], 255);
                    // printf("%lf %lf %lf %lf\n", circleX - radius, circleY - radius, circleX + radius, circleY + radius);
                }
                circleX += height * diamondAspect / self.diamondWidth;
            }
            circleY -= height / self.diamondHeight;
        }
        if (renderDimensions) {
            /* render dot dimensions */
            tt_setColor(TT_COLOR_TEXT);
            turtlePenSize(2);
            turtleGoto(diamondXLeft - 25, diamondY);
            turtlePenDown();
            turtleGoto(diamondXLeft - 15, diamondY);
            turtleGoto(diamondXLeft - 20, diamondY);
            turtleGoto(diamondXLeft - 20, diamondY + height);
            turtleGoto(diamondXLeft - 25, diamondY + height);
            turtleGoto(diamondXLeft - 15, diamondY + height);
            turtlePenUp();
            turtleTextWriteStringf(diamondXLeft - 30, (diamondY * 2 + height) / 2, 15, 100, "%d", self.diamondHeight);

            turtleGoto(diamondXLeft, diamondY - 25);
            turtlePenDown();
            turtleGoto(diamondXLeft, diamondY - 15);
            turtleGoto(diamondXLeft, diamondY - 20);
            turtleGoto(diamondXRight, diamondY - 20);
            turtleGoto(diamondXRight, diamondY - 25);
            turtleGoto(diamondXRight, diamondY - 15);
            turtlePenUp();
            turtleTextWriteStringf((diamondXRight + diamondXLeft) / 2, diamondY - 35, 15, 50, "%d", self.diamondWidth);
        }
    }
}

/* main canvas for color mode */
void renderColor() {

}

void render() {
    if (self.mode == DIAMOND_UI_MODE_IMAGE) {
        renderDotImage(-50, 0, 240, 1, 1);
    } else {
        renderColor();
    }
    /* sidebar */
    tt_setColor(TT_COLOR_BACKGROUND_COMPLEMENT);
    turtleRectangle(238, 180, 320, -180);
    if (self.mode == DIAMOND_UI_MODE_IMAGE) {
        self.imageButton -> color[TT_COLOR_SLOT_BUTTON] = TT_COLOR_BACKGROUND_COMPLEMENT;
        self.imageButton -> color[TT_COLOR_SLOT_BUTTON_SELECT] = TT_COLOR_BACKGROUND_COMPLEMENT;
        self.imageButton -> color[TT_COLOR_SLOT_BUTTON_TEXT] = TT_COLOR_COMPONENT_ALTERNATE;
        self.colorButton -> color[TT_COLOR_SLOT_BUTTON] = TT_COLOR_COMPONENT;
        self.colorButton -> color[TT_COLOR_SLOT_BUTTON_SELECT] = TT_COLOR_COMPONENT_HIGHLIGHT;
        self.colorButton -> color[TT_COLOR_SLOT_BUTTON_TEXT] = TT_COLOR_TEXT_ALTERNATE;
        if (self.colorButton -> value) {
            self.colorButton -> value = 0;
            self.mode = DIAMOND_UI_MODE_COLOR;
        }
    } else if (self.mode == DIAMOND_UI_MODE_COLOR) {
        self.imageButton -> color[TT_COLOR_SLOT_BUTTON] = TT_COLOR_COMPONENT;
        self.imageButton -> color[TT_COLOR_SLOT_BUTTON_SELECT] = TT_COLOR_COMPONENT_HIGHLIGHT;
        self.imageButton -> color[TT_COLOR_SLOT_BUTTON_TEXT] = TT_COLOR_TEXT_ALTERNATE;
        if (self.imageButton -> value) {
            self.imageButton -> value = 0;
            self.mode = DIAMOND_UI_MODE_IMAGE;
        }
        self.colorButton -> color[TT_COLOR_SLOT_BUTTON] = TT_COLOR_BACKGROUND_COMPLEMENT;
        self.colorButton -> color[TT_COLOR_SLOT_BUTTON_SELECT] = TT_COLOR_BACKGROUND_COMPLEMENT;
        self.colorButton -> color[TT_COLOR_SLOT_BUTTON_TEXT] = TT_COLOR_COMPONENT_ALTERNATE;
    }
    /* render original image (preview) */
    if (self.originalTexture != -1) {
        double originalAspect = (double) self.originalWidth / self.originalHeight;
        double previewX = 305;
        double previewY = 90;
        if (self.mode == DIAMOND_UI_MODE_IMAGE) {
            /* render original image */
            turtleTexture(self.originalTexture, previewX - 50 * originalAspect, previewY, previewX, previewY + 50, 0, 255, 255, 255);
        } else {
            /* render dot image */
            renderDotImage((previewX * 2 - 50 * originalAspect) / 2, previewY + 25, 50, 0, 0);
        }
        tt_setColor(TT_COLOR_COMPONENT_ALTERNATE);
        turtleTextWriteString("Preview", (previewX + previewX - 50 * originalAspect) / 2, previewY + 50 + 15, 10, 50);
        self.resolutionSlider -> x = (previewX + previewX - 50 * originalAspect) / 2;
        self.resizeModeDropdown -> x = (previewX + previewX - 50 * originalAspect) / 2;
    }
}

void mouseTick() {
    if (turtleKeyPressed(GLFW_KEY_LEFT)) {
        if (self.keys[DIAMOND_KEY_LEFT_ARROW] < 2) {
            if (self.keys[DIAMOND_KEY_LEFT_ARROW] == 0) {
                self.keys[DIAMOND_KEY_LEFT_ARROW] = 10;
            } else {
                self.keys[DIAMOND_KEY_LEFT_ARROW] = 2;
            }
            if (self.resolutionSlider -> value > self.resolutionSlider -> range[0]) {
                self.resolutionSlider -> value--;
            }
        } else if (self.keys[DIAMOND_KEY_LEFT_ARROW] >= 10) {
            self.keys[DIAMOND_KEY_LEFT_ARROW]++;
            if (self.keys[DIAMOND_KEY_LEFT_ARROW] > 40) {
                self.keys[DIAMOND_KEY_LEFT_ARROW] = 1;
            }
        } else {
            self.keys[DIAMOND_KEY_LEFT_ARROW]++;
            if (self.keys[DIAMOND_KEY_LEFT_ARROW] > 3) {
                self.keys[DIAMOND_KEY_LEFT_ARROW] = 1;
            }
        }
    } else {
        self.keys[DIAMOND_KEY_LEFT_ARROW] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_RIGHT)) {
        if (self.keys[DIAMOND_KEY_RIGHT_ARROW] < 2) {
            if (self.keys[DIAMOND_KEY_RIGHT_ARROW] == 0) {
                self.keys[DIAMOND_KEY_RIGHT_ARROW] = 10;
            } else {
                self.keys[DIAMOND_KEY_RIGHT_ARROW] = 2;
            }
            if (self.resolutionSlider -> value < self.resolutionSlider -> range[1]) {
                self.resolutionSlider -> value++;
            }
        } else if (self.keys[DIAMOND_KEY_RIGHT_ARROW] >= 10) {
            self.keys[DIAMOND_KEY_RIGHT_ARROW]++;
            if (self.keys[DIAMOND_KEY_RIGHT_ARROW] > 40) {
                self.keys[DIAMOND_KEY_RIGHT_ARROW] = 1;
            }
        } else {
            self.keys[DIAMOND_KEY_RIGHT_ARROW]++;
            if (self.keys[DIAMOND_KEY_RIGHT_ARROW] > 3) {
                self.keys[DIAMOND_KEY_RIGHT_ARROW] = 1;
            }
        }
    } else {
        self.keys[DIAMOND_KEY_RIGHT_ARROW] = 0;
    }
}

void parseRibbonOutput() {
    if (tt_ribbon.output[0] == 0) {
        return;
    }
    tt_ribbon.output[0] = 0;
    if (tt_ribbon.output[1] == 0) { // File
        if (tt_ribbon.output[2] == 1) { // New
            list_clear(osToolsFileDialog.selectedFilenames);
            printf("New\n");
        }
        if (tt_ribbon.output[2] == 2) { // Save
            if (osToolsFileDialog.selectedFilenames -> length == 0) {
                if (osToolsFileDialogSave(OSTOOLS_FILE_DIALOG_FILE, "Save.txt", NULL) != -1) {
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
                }
            } else {
                printf("Saved to: %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
        if (tt_ribbon.output[2] == 3) { // Save As...
            list_clear(osToolsFileDialog.selectedFilenames);
            if (osToolsFileDialogSave(OSTOOLS_FILE_DIALOG_FILE, "Save.txt", NULL) != -1) {
                printf("Saved to: %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
        if (tt_ribbon.output[2] == 4) { // Open
            list_clear(osToolsFileDialog.selectedFilenames);
            if (osToolsFileDialogOpen(OSTOOLS_FILE_DIALOG_SINGLE_SELECT, OSTOOLS_FILE_DIALOG_FILE, "", NULL) != -1) {
                import(osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
    }
    if (tt_ribbon.output[1] == 1) { // Edit
        if (tt_ribbon.output[2] == 1) { // Undo
            printf("Undo\n");
        }
        if (tt_ribbon.output[2] == 2) { // Redo
            printf("Redo\n");
        }
        if (tt_ribbon.output[2] == 3) { // Cut
            osToolsClipboardSetText("test123");
            printf("Cut \"test123\" to clipboard!\n");
        }
        if (tt_ribbon.output[2] == 4) { // Copy
            osToolsClipboardSetText("test345");
            printf("Copied \"test345\" to clipboard!\n");
        }
        if (tt_ribbon.output[2] == 5) { // Paste
            osToolsClipboardGetText();
            printf("Pasted \"%s\" from clipboard!\n", osToolsClipboard.text);
        }
    }
    if (tt_ribbon.output[1] == 2) { // View
        if (tt_ribbon.output[2] == 1) { // Change theme
            printf("Change theme\n");
            if (tt_theme == TT_THEME_DARK) {
                turtleBgColor(36, 30, 32);
                turtleToolsSetTheme(TT_THEME_COLT);
            } else if (tt_theme == TT_THEME_COLT) {
                turtleBgColor(212, 201, 190);
                turtleToolsSetTheme(TT_THEME_NAVY);
            } else if (tt_theme == TT_THEME_NAVY) {
                turtleBgColor(255, 255, 255);
                turtleToolsSetTheme(TT_THEME_LIGHT);
            } else if (tt_theme == TT_THEME_LIGHT) {
                turtleBgColor(30, 30, 30);
                turtleToolsSetTheme(TT_THEME_DARK);
            }
        } 
        if (tt_ribbon.output[2] == 2) { // GLFW
            printf("GLFW settings\n");
        } 
    }
}

void parsePopupOutput(GLFWwindow *window) {
    if (tt_popup.output[0] == 0) {
        return;
    }
    tt_popup.output[0] = 0; // untoggle
    if (tt_popup.output[1] == 0) { // cancel
        turtle.close = 0;
        glfwSetWindowShouldClose(window, 0);
    }
    if (tt_popup.output[1] == 1) { // close
        turtle.popupClose = 1;
    }
}

int main(int argc, char *argv[]) {
    /* Initialise glfw */
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA (Anti-Aliasing) with 4 samples (must be done before window is created (?))

    /* Create a windowed mode window and its OpenGL context */
    const GLFWvidmode *monitorSize = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int32_t windowHeight = monitorSize -> height;
    double optimizedScalingFactor = 0.8; // Set this number to 1 on windows and 0.8 on Ubuntu for maximum compatibility (fixes issue with incorrect stretching)
    #ifdef OS_WINDOWS
    optimizedScalingFactor = 1;
    #endif
    #ifdef OS_LINUX
    optimizedScalingFactor = 0.8;
    #endif
    GLFWwindow *window = glfwCreateWindow(windowHeight * 16 / 9 * optimizedScalingFactor, windowHeight * optimizedScalingFactor, "diamond", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, windowHeight * 16 / 9 * 0.4, windowHeight * 0.4, windowHeight * 16 / 9 * optimizedScalingFactor, windowHeight * optimizedScalingFactor);
    /* initialise logo */
    GLFWimage icon;
    int32_t iconChannels;
    char constructedFilepath[5120];
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "images/thumbnail.png");
    uint8_t *iconPixels = stbi_load(constructedFilepath, &icon.width, &icon.height, &iconChannels, 4); // 4 color channels for RGBA
    if (iconPixels != NULL) {
        icon.pixels = iconPixels;
        glfwSetWindowIcon(window, 1, &icon);
        glfwPollEvents(); // update taskbar icon correctly on windows - https://github.com/glfw/glfw/issues/2753
        free(iconPixels);
    } else {
        printf("Could not load thumbnail %s\n", constructedFilepath);
    }

    /* initialise turtle */
    turtleInit(window, -320, -180, 320, 180);
    #ifdef OS_LINUX
    glfwSetWindowPos(window, 0, 36);
    #endif
    if (optimizedScalingFactor > 0.85) {
        glfwSetWindowSize(window, windowHeight * 16 / 9 * 0.85, windowHeight * 0.85); // doing it this way ensures the window spawns in the top left of the monitor and fixes resizing limits
    } else {
        glfwSetWindowSize(window, windowHeight * 16 / 9 * optimizedScalingFactor, windowHeight * optimizedScalingFactor);
    }
    /* initialise osTools */
    osToolsInit(argv[0], window); // must include argv[0] to get executableFilepath, must include GLFW window for copy paste and cursor functionality
    osToolsFileDialogAddGlobalExtension("png"); // add png to extension restrictions
    osToolsFileDialogAddGlobalExtension("jpg"); // add jpg to extension restrictions
    osToolsFileDialogAddGlobalExtension("bmp"); // add bmp to extension restrictions
    /* initialise turtleText */
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/roberto.tgl");
    turtleTextInit(constructedFilepath);
    /* initialise turtleTools ribbon */
    turtleToolsSetTheme(TT_THEME_DARK); // dark theme preset
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/ribbonConfig.txt");
    ribbonInit(constructedFilepath);
    // list_t *ribbonConfig = list_init();
    // list_append(ribbonConfig, (unitype) "File, New, Save, Save As..., Open", 's');
    // list_append(ribbonConfig, (unitype) "Edit, Undo, Redo, Cut, Copy, Paste", 's');
    // list_append(ribbonConfig, (unitype) "View, Change Theme, GLFW", 's');
    // ribbonInitList(ribbonConfig);
    /* initialise turtleTools popup */
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/popupConfig.txt");
    // popupInit(constructedFilepath);
    // list_t *popupConfig = list_init();
    // list_append(popupConfig, (unitype) "Are you sure you want to close?", 's');
    // list_append(popupConfig, (unitype) "Cancel", 's');
    // list_append(popupConfig, (unitype) "Close", 's');
    // popupInitList(popupConfig);

    init();

    uint32_t tps = 120; // ticks per second (locked to fps in this case)
    uint64_t tick = 0; // count number of ticks since application started
    clock_t start, end;
    while (turtle.close == 0) {
        start = clock();
        turtleGetMouseCoords();
        turtleClear();
        transform();
        render();
        mouseTick();
        turtleToolsUpdate(); // update turtleTools
        parseRibbonOutput(); // user defined function to use ribbon
        parsePopupOutput(window); // user defined function to use popup
        turtleUpdate(); // update the screen
        end = clock();
        while ((double) (end - start) / CLOCKS_PER_SEC < (1.0 / tps)) {
            end = clock();
        }
        tick++;
    }
    turtleFree();
    glfwTerminate();
    return 0;
}
