#include <ctime>
#include <cstdio>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <glut.h>
#include <windows.h>     // For Windows API and PlaySound
#include <mmsystem.h>    // For PlaySound (winmm.lib needed)
#define M_PI 3.14159265358979323846


// Function to play background music
static void playMusic(const char* filePath) {
    if (!PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP)) {
        printf("Failed to play sound!\n");  // Debugging message
    }
}

// Function to stop any currently playing music
static void stopMusic() {
    PlaySound(NULL, NULL, SND_ASYNC);
}

// Function to play sound effects
static void playSoundEffect(const char* filePath) {
    PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC);
}

// Global Variables
static float playerX = -0.8f;  // Starting X position for the player
float playerY = 0.0f;        // Player's Y position (for jumping)
bool isJumping = false;      // Whether the player is in the air
bool isDucking = false;      // Whether the player is ducking
float jumpVelocity = 0.05f;  // Velocity for jumping
float gravity = 0.002f;      // Gravity effect
float collectibleX = 1.5f;   // X position of the collectible
float collectibleY = 0.0f;  // Ground level, adjust Y position to match the ground
bool collectibleActive = true; // Whether the collectible is active or has been collected
float powerUpX = 1.5f;       // X position of a power-up
float gameSpeed = 0.01f;     // Speed of the game (increases over time)
int lives = 5;               // Player lives
int score = 0;               // Player score
int gameTime = 60;          // Total game time (120 seconds, or 2 minutes)
clock_t startTime;           // Start time of the game
float powerUpRotationAngle = 0.0f;  // Rotation angle for power-ups
float collectiblePulseScale = 1.0f;  // Scale factor for collectibles
bool increasingScale = true;  // To alternate scaling for the pulse effect
bool hasMagnet = false;           // Track if player has the magnet power-up
bool isInvincible = false;        // Track if player is invincible
clock_t powerUpStartTime;        // Track when the power-up was acquired
bool isKnockedBack = false;
bool isReadjusting = false;
float knockbackAmount = 0.05f;  // The amount of knockback
float readjustSpeed = 0.01f;    // Speed to move the player back to the original position
float knockbackStrength = 0.02f;  // How much the player is knocked back
int knockbackDuration = 30;       // How many frames the knockback lasts
int knockbackTimer = 0;           // Timer to keep track of knockback
int starSpawnCounter = 0;  // Counter for star spawning
static bool gameEnd = false;   // Flag for when the timer runs out
static bool gameLose = false;  // Flag for when player loses all health

struct Star {
    float x;
    float y;
    float size;
};

std::vector<Star> stars;  // Vector to hold stars

// Obstacle Structure
struct Obstacle {
    float x;  // X position
    float y;  // Y position (ground or slightly above)
    float width, height;  // Dimensions of the obstacle
    bool hasHitPlayer = false;  // Track if this obstacle has already hit the player
};

std::vector<Obstacle> obstacles;  // List of obstacles

struct Collectible {
    float x, y;        // Position of the collectible
    float size;        // Size of the collectible
    bool active;       // Whether the collectible is active or collected
};

std::vector<Collectible> collectibles;  // Store multiple 

struct PowerUp {
    float x, y;        // Position of the power-up
    float size;        // Size of the power-up
    bool active;       // Whether the power-up is active
    int type;          // Type of power-up (1 for magnet, 2 for invincibility)
};

std::vector<PowerUp> powerUps;  // Store multiple power-ups



// Function to draw a heart shape
static void DrawHeart(float x, float y, float size) {
    glBegin(GL_POLYGON);
    glColor3f(1.0f, 0.0f, 0.0f);  // Red color for health

    // Heart shape vertices
    for (float angle = 0; angle < 2 * M_PI; angle += 0.01f) {
        float dx = size * (16 * pow(sin(angle), 3));
        float dy = size * (13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle));
        glVertex2f(x + dx, y + dy);
    }

    glEnd();
}

// Function to draw the health bar (heart shape)
static void DrawHealthBar() {
    glPushMatrix();
    glTranslatef(-0.9f, 0.8f, 0.0f);  // Position at the top-left

    // Draw lives using heart shapes (one for each life)
    for (int i = 0; i < lives; i++) {
        DrawHeart(i * 0.17f, 0.0f, 0.005f);  // Increased spacing to 0.08f
    }

    glPopMatrix();
}

// Function to display score and time at the top-right of the screen
static void DrawScoreAndTime() {

    // Set the text color to a bright color (like yellow or white) for better visibility
    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color for score and time

    // Format score text
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);

    // Format time text
    char timeText[50];
    sprintf(timeText, "Time: %d", gameTime);

    // Display score on the top-right
    glRasterPos2f(0.5f, 0.85f);  // Position for score
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Display time next to the score
    glRasterPos2f(0.8f, 0.85f);  // Position for time
    for (char* c = timeText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Optional: Add a glow effect around the text (this part is a concept, actual implementation may vary)
    glColor3f(1.0f, 1.0f, 1.0f);  // White for the glow effect
    glRasterPos2f(0.5f + 0.005f, 0.85f + 0.005f);  // Offset for glow
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glRasterPos2f(0.8f + 0.005f, 0.85f + 0.005f);  // Offset for glow
    for (char* c = timeText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// Function to draw the player as an astronaut
static void DrawPlayer() {
    glPushMatrix();
    glTranslatef(playerX - 0.1f, playerY - 0.7f, 0.0f);  // Adjust playerY for jumping and standing on the ground

    if (isDucking) {
        glScalef(1.0f, 0.5f, 1.0f);  // Shrink player when ducking
    }

    // Draw the astronaut suit (body)
    glBegin(GL_QUADS);  // Body (quad)
    glColor3f(0.0f, 0.0f, 1.0f);  // Blue color for the suit
    glVertex2f(-0.05f, 0.0f);  // Lower vertex aligned with the ground
    glVertex2f(0.05f, 0.0f);
    glVertex2f(0.05f, 0.1f);
    glVertex2f(-0.05f, 0.1f);
    glEnd();

    // Draw the helmet (cap) - a larger arc to simulate a dome shape
    glBegin(GL_TRIANGLE_FAN);  // Helmet (dome)
    glColor3f(1.0f, 1.0f, 1.0f);  // White color for the helmet
    glVertex2f(0.0f, 0.15f);  // Center of the helmet
    for (int i = 0; i <= 30; i++) {
        float theta = (float)i / 30.0f * 3.14159f;  // Half circle for the dome
        glVertex2f(0.04f * cos(theta), 0.15f + 0.04f * sin(theta));  // Radius of 0.04
    }
    glEnd();

    // Draw the head (skin)
    glBegin(GL_TRIANGLES);  // Head (triangle)
    glColor3f(1.0f, 0.85f, 0.7f);  // Skin color
    glVertex2f(-0.03f, 0.1f);
    glVertex2f(0.03f, 0.1f);
    glVertex2f(0.0f, 0.15f);
    glEnd();

    // Draw the visor (optional, can be a simple rectangle)
    glBegin(GL_QUADS);  // Visor
    glColor3f(0.0f, 0.0f, 0.0f);  // Black color for the visor
    glVertex2f(-0.025f, 0.1f);
    glVertex2f(0.025f, 0.1f);
    glVertex2f(0.015f, 0.12f);
    glVertex2f(-0.015f, 0.12f);
    glEnd();

    // Draw a stripe on the suit
    glBegin(GL_QUADS);  // Stripe
    glColor3f(1.0f, 0.0f, 0.0f);  // Red color for the stripe
    glVertex2f(-0.05f, 0.05f);
    glVertex2f(0.05f, 0.05f);
    glVertex2f(0.05f, 0.06f);
    glVertex2f(-0.05f, 0.06f);
    glEnd();

    glPopMatrix();
}

static void DrawPowerUps() {
    for (const auto& powerUp : powerUps) {
        if (powerUp.active) {
            glPushMatrix();
            glTranslatef(powerUp.x, powerUp.y, 0.0f);
            glScalef(powerUp.size * 1.5f, powerUp.size * 1.5f, 1.0f);  // Increase the scaling factor

            // Rotate the power-up around its center
            glRotatef(powerUpRotationAngle, 0.0f, 0.0f, 1.0f);

            // Draw magnet power-up
            if (powerUp.type == 1) {
                glColor3f(1.0f, 0.0f, 0.0f);  // Red color for magnet

                // 1. Draw left rectangle (left bar of the magnet)
                glBegin(GL_QUADS);
                glVertex2f(-0.8f, 0.0f);  // Bottom-left
                glVertex2f(-0.6f, 0.0f);  // Bottom-right
                glVertex2f(-0.6f, 1.0f);  // Top-right
                glVertex2f(-0.8f, 1.0f);  // Top-left
                glEnd();

                // 2. Draw right rectangle (right bar of the magnet)
                glBegin(GL_QUADS);
                glVertex2f(0.6f, 0.0f);  // Bottom-left
                glVertex2f(0.8f, 0.0f);  // Bottom-right
                glVertex2f(0.8f, 1.0f);  // Top-right
                glVertex2f(0.6f, 1.0f);  // Top-left
                glEnd();

                glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color for the top curves

                // 3. Draw left arc (curve on the left)
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(-0.7f, 1.0f);  // Center of the curve
                for (int i = 180; i <= 270; i += 5) {
                    float theta = i * M_PI / 180.0f;
                    glVertex2f(-0.7f + 0.3f * cos(theta), 1.0f + 0.3f * sin(theta));  // Draw arc
                }
                glEnd();

                // 4. Draw right arc (curve on the right)
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(0.7f, 1.0f);  // Center of the curve
                for (int i = 270; i <= 360; i += 5) {
                    float theta = i * M_PI / 180.0f;
                    glVertex2f(0.7f + 0.3f * cos(theta), 1.0f + 0.3f * sin(theta));  // Draw arc
                }
                glEnd();
            }

            // Draw invincibility power-up (green upward arrow)
            else if (powerUp.type == 2) {
                glColor3f(0.0f, 1.0f, 0.0f);  // Green color for invincibility

                // 1. Draw arrowhead (triangle)
                glBegin(GL_TRIANGLES);
                glVertex2f(-0.6f, 0.0f);  // Left point
                glVertex2f(0.6f, 0.0f);   // Right point
                glVertex2f(0.0f, 1.2f);   // Top point
                glEnd();

                // 2. Draw arrow body (center rectangle)
                glBegin(GL_QUADS);
                glVertex2f(-0.3f, -0.6f);
                glVertex2f(0.3f, -0.6f);
                glVertex2f(0.3f, 0.0f);
                glVertex2f(-0.3f, 0.0f);
                glEnd();

                // 3. Draw left wing (rectangle on the left)
                glBegin(GL_QUADS);
                glVertex2f(-0.8f, -0.3f);
                glVertex2f(-0.3f, -0.3f);
                glVertex2f(-0.3f, -0.6f);
                glVertex2f(-0.8f, -0.6f);
                glEnd();

                // 4. Draw right wing (rectangle on the right)
                glBegin(GL_QUADS);
                glVertex2f(0.3f, -0.3f);
                glVertex2f(0.8f, -0.3f);
                glVertex2f(0.8f, -0.6f);
                glVertex2f(0.3f, -0.6f);
                glEnd();
            }

            glPopMatrix();
        }
    }
}

// Function to draw obstacles
static void DrawObstacles() {
    for (auto& obstacle : obstacles) {
        glPushMatrix();
        glTranslatef(obstacle.x, obstacle.y, 0.0f);

        // Draw the main body of the obstacle (a rectangle)
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);  // Red color for the obstacle
        glVertex2f(0.0f, 0.0f);
        glVertex2f(obstacle.width, 0.0f);
        glVertex2f(obstacle.width, obstacle.height);
        glVertex2f(0.0f, obstacle.height);
        glEnd();

        // Draw a shadow beneath the obstacle
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);  // Semi-transparent black for shadow
        glVertex2f(-0.05f, -0.02f);  // Shadow offset to give depth
        glVertex2f(obstacle.width + 0.05f, -0.02f);
        glVertex2f(obstacle.width + 0.05f, -0.05f);
        glVertex2f(-0.05f, -0.05f);
        glEnd();

        // Draw an additional decorative element (like a stripe or a star on the obstacle)
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.85f, 0.0f);  // Yellow stripe color
        glVertex2f(obstacle.width / 2, obstacle.height); // Top point
        glVertex2f(obstacle.width / 4, obstacle.height - 0.1f); // Bottom left point
        glVertex2f(3 * obstacle.width / 4, obstacle.height - 0.1f); // Bottom right point
        glEnd();

        glPopMatrix();
    }
}

/*static void DrawCollectibles() {
    for (const auto& collectible : collectibles) {
        if (collectible.active) {
            glPushMatrix();
            glTranslatef(collectible.x, collectible.y, 0.0f);

            // Apply pulsing effect (scaling the collectible)
            glScalef(collectiblePulseScale, collectiblePulseScale, 1.0f);

            // Draw the collectible (yellow circle for now)
            glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color
            glBegin(GL_POLYGON);
            for (int i = 0; i < 20; i++) {
                float theta = 2.0f * 3.1415926f * float(i) / float(20);
                float dx = collectible.size * cosf(theta);
                float dy = collectible.size * sinf(theta);
                glVertex2f(dx, dy);
            }
            glEnd();

            glPopMatrix();
        }
    }
}
*/

// Function to draw a star (as a point or small polygon)
// Function to draw a star
static void DrawStar(float x, float y, float size) {
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f);  // White color for the star
    for (int i = 0; i < 5; i++) {
        float angle = i * (2.0f * M_PI / 5);  // Angle for each vertex
        float xOuter = x + size * cos(angle);  // Outer vertex X
        float yOuter = y + size * sin(angle);  // Outer vertex Y

        angle += M_PI / 5;  // Adjust for inner vertex
        float xInner = x + (size * 0.5f) * cos(angle);  // Inner vertex X
        float yInner = y + (size * 0.5f) * sin(angle);  // Inner vertex Y

        glVertex2f(xOuter, yOuter);  // Outer vertex
        glVertex2f(xInner, yInner);   // Inner vertex
    }
    glEnd();
}

//Function to draw a star(as a point or small polygon)
static void DrawStar2(float x, float y, float size) {
    glBegin(GL_POLYGON);
    glColor3f(1.0f, 1.0f, 1.0f);  // White color for the star
    for (int i = 0; i < 5; i++) {
        float theta = 2.0f * M_PI * i / 5;
        glVertex2f(x + size * cos(theta), y + size * sin(theta));
    }
    glEnd();
}

// Function to initialize stars
static void InitializeStars(int numStars) {
    stars.clear();  // Clear any existing stars
    for (int i = 0; i < numStars; i++) {
        Star star;
        star.x = ((rand() % 200) - 100) / 100.0f;  // Random X position between -1 and 1
        star.y = ((rand() % 200) - 100) / 100.0f;  // Random Y position between -1 and 1
        star.size = 0.005f * (rand() % 3 + 1);     // Random size (small to medium)
        stars.push_back(star);                      // Add star to the vector
    }
}

// Function to draw the background including stars
static void DrawBackground() {
    for (const auto& star : stars) {
        DrawStar2(star.x, star.y, star.size);
    }
}

// Function to draw an outer ring around the collectible
static void DrawOuterRing(float x, float y, float size) {
    glBegin(GL_LINE_LOOP);  // Draw as a line loop for the outer ring
    glColor3f(1.0f, 0.8f, 0.0f);  // Slightly darker yellow for the ring
    for (int i = 0; i < 20; i++) {
        float theta = 2.0f * M_PI * float(i) / float(20);
        float dx = (size + 0.02f) * cosf(theta);  // Slightly larger radius for the ring
        float dy = (size + 0.02f) * sinf(theta);
        glVertex2f(x + dx, y + dy);
    }
    glEnd();
}

// Function to draw the collectibles with enhanced visuals
static void DrawCollectibles() {
    for (const auto& collectible : collectibles) {
        if (collectible.active) {
            glPushMatrix();
            glTranslatef(collectible.x, collectible.y, 0.0f);

            // Apply pulsing effect (scaling the collectible)
            glScalef(collectiblePulseScale, collectiblePulseScale, 1.0f);

            // Draw the main collectible (yellow circle)
            glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color for the circle
            glBegin(GL_POLYGON);
            for (int i = 0; i < 20; i++) {
                float theta = 2.0f * M_PI * float(i) / float(20);
                float dx = collectible.size * cosf(theta);
                float dy = collectible.size * sinf(theta);
                glVertex2f(dx, dy);
            }
            glEnd();

            // Draw an outer ring around the collectible
            DrawOuterRing(0.0f, 0.0f, collectible.size);

            // Draw a star on top of the collectible
            DrawStar(0.0f, 0.0f, collectible.size * 0.5f);  // Star size is half of the collectible size

            glPopMatrix();
        }
    }
}

// Function to draw a simple asteroid (using a polygon)
static void DrawAsteroid(float x, float y, float size) {
    glBegin(GL_POLYGON);
    glColor3f(0.5f, 0.5f, 0.5f);  // Gray color for the asteroid
    for (int i = 0; i < 7; i++) {
        float theta = 2.0f * M_PI * i / 7;
        glVertex2f(x + size * cos(theta), y + size * sin(theta));
    }
    glEnd();
}

// Function to draw upper and lower boundaries with space objects
static void DrawBoundaries() {
    // Upper boundary
    for (int i = 0; i < 4; i++) {
        DrawAsteroid(-0.9f + i * 0.5f, 0.96f, 0.05f);  // Placing asteroids along the upper boundary
    }

    // Lower boundary
    for (int i = 0; i < 4; i++) {
        DrawAsteroid(-0.9f + i * 0.5f, -0.9f, 0.05f);  // Placing asteroids along the lower boundary
    }
}

// Function to draw a glowing moon
static void DrawMoon(float x, float y, float size) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(gameTime * 10, 0.0f, 0.0f, 1.0f);  // Rotate the moon slowly over time

    // Draw the main body of the moon (gray color)
    glBegin(GL_POLYGON);
    glColor3f(0.8f, 0.8f, 0.8f);  // Light gray color for the moon
    for (int i = 0; i < 30; i++) {
        float theta = 2.0f * M_PI * i / 30;
        glVertex2f(size * cos(theta), size * sin(theta));
    }
    glEnd();

    // Draw a glowing effect around the moon
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 0.8f, 0.4f);  // Semi-transparent light yellow color for glow
    glVertex2f(0.0f, 0.0f);  // Center point
    for (int i = 0; i <= 30; i++) {
        float theta = 2.0f * M_PI * i / 30;
        glVertex2f(size * 1.5f * cos(theta), size * 1.5f * sin(theta));  // Slightly larger than the moon
    }
    glEnd();

    glPopMatrix();
}

// Function to render bitmap text
void renderBitmapString(float x, float y, void* font, const char* string) {
    glRasterPos2f(x, y);
    for (const char* c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// Function to handle game end screen
static void DisplayGameEnd(const char* message) {
    glClear(GL_COLOR_BUFFER_BIT);

    // Background color for the end screen
    glColor3f(0.0f, 0.0f, 0.0f);  // Full black background
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Center the text horizontally and vertically
    glColor3f(0.8f, 0.0f, 0.0f);  // Dark, blood-red color for the text
    renderBitmapString(-0.16f, 0.2f, GLUT_BITMAP_TIMES_ROMAN_24, message);  // Game End or Lose message

    // Display the score in a slightly smaller font, also centered
    glColor3f(0.9f, 0.9f, 0.9f);  // White color for score text
    char scoreMessage[50];
    sprintf(scoreMessage, "Your final score is: %d", score);
    renderBitmapString(-0.23f, 0.0f, GLUT_BITMAP_HELVETICA_18, scoreMessage);

    // Add a decorative border
    glColor3f(1.0f, 1.0f, 1.0f);  // White border
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.6f, -0.4f);
    glVertex2f(0.6f, -0.4f);
    glVertex2f(0.6f, 0.6f);
    glVertex2f(-0.6f, 0.6f);
    glEnd();

    glFlush();
    glutSwapBuffers();
}

// Update background animations
static void UpdateBackgroundAnimations() {
    // Draw stars (if applicable, you can call your star drawing function here)

    // Add planets and other space objects that slightly move or rotate
    DrawMoon(-0.7f, 0.5f, 0.1f);  // Example of adding a moon

    // You can add more moons or space objects if needed
}

static void UpdateAnimations() {
    // Rotate power-ups
    powerUpRotationAngle += 1.0f + (gameSpeed * 0.1f);  // Increase rotation based on game speed
    if (powerUpRotationAngle >= 360.0f) {
        powerUpRotationAngle = 0.0f;  // Reset to avoid overflow
    }

    // Pulse effect for collectibles (scale up and down)
    if (increasingScale) {
        collectiblePulseScale += 0.01f + (gameSpeed * 0.001f);  // Increase scale based on speed
        if (collectiblePulseScale >= 1.2f) {
            increasingScale = false;  // Start decreasing when max scale is reached
        }
    }
    else {
        collectiblePulseScale -= 0.01f + (gameSpeed * 0.001f);  // Decrease scale based on speed
        if (collectiblePulseScale <= 0.8f) {
            increasingScale = true;  // Start increasing again when min scale is reached
        }
    }
}

static void MoveCollectibles() {
    for (auto& collectible : collectibles) {
        if (collectible.active) {
            collectible.x -= gameSpeed;

            // Deactivate or reset collectible if it goes off-screen
            if (collectible.x < -1.0f) {
                collectible.active = false;  // Deactivate the collectible
            }
        }
    }
}

static void MovePowerUps() {
    for (auto& powerUp : powerUps) {
        if (powerUp.active) {
            powerUp.x -= gameSpeed;  // Move power-up downwards
            if (powerUp.x < -1.0f) {
                powerUp.active = false;  // Deactivate if it goes off-screen
            }
        }
    }
}

static void SpawnCollectibles() {
    if (rand() % 80 == 0) {  // Randomize the spawning frequency
        Collectible newCollectible;
        newCollectible.x = 1.0f;  // Start at the right edge of the screen

        // Randomly decide whether to spawn on the ground or in the air
        if (rand() % 2 == 0) {
            newCollectible.y = -0.6f;  // Ground level
        }
        else {
            newCollectible.y = 0.5f; // High in the air
        }

        newCollectible.size = 0.05f;  // Default size
        newCollectible.active = true;

        collectibles.push_back(newCollectible);  // Add the collectible to the vector
    }
}

// Function to spawn obstacles and collectables randomly with spacing
static void SpawnObstacles() {
    // Ensure a minimum distance between consecutive obstacles
    if (!obstacles.empty() && obstacles.back().x > 0.5f) {
        return;  // If the last obstacle is too close, skip spawning
    }

    Obstacle obs;
    obs.x = 1.0f;  // Spawn at the right edge of the screen

    // Randomly set obstacle height to either ground level or slightly above the player
    if (rand() % 3 == 0) {
        obs.y = -0.7f;  // Ground level (obstacle sits above the grass but aligned with the player)
        obs.height = 0.2f;  // Small obstacle on the ground (for jumping over)
    }
    else {
        obs.y = -0.5f;  // Positioned slightly above the player (requires ducking)
        obs.height = 0.25f;  // Taller obstacle (for ducking under)
    }

    obs.width = 0.1f;  // Fixed width

    obstacles.push_back(obs);
}

static void SpawnPowerUps() {
    if (rand() % 180 == 0) {  // Randomize the spawning frequency
        PowerUp newPowerUp;
        newPowerUp.x = 1.0f;  // Start at the right edge of the screen
        if (rand() % 2 == 0) {
            newPowerUp.y = -0.6f;  // Ground level
        }
        else {
            newPowerUp.y = 0.5f; // High in the air
        }
        newPowerUp.size = 0.05f;  // Default size
        newPowerUp.active = true;

        // Randomly assign a type (1 for magnet, 2 for invincibility)
        newPowerUp.type = (rand() % 2) + 1;  // Either 1 or 2
        powerUps.push_back(newPowerUp);  // Add the power-up to the vector
    }
}

// Function to move obstacles toward the player and remove them when off-screen
static void MoveObstacles() {
    for (auto it = obstacles.begin(); it != obstacles.end();) {
        it->x -= gameSpeed;  // Move obstacle to the left

        // Remove obstacles that go off-screen
        if (it->x < -1.0f) {
            it = obstacles.erase(it);
        }
        else {
            ++it;
        }
    }
}

// Function to handle jumping mechanics with speed adjustments
static void JumpMechanics() {
    if (isJumping) {
        playerY += jumpVelocity * gameSpeed * 100;  // Move the player upwards faster as gameSpeed increases
        jumpVelocity -= gravity * gameSpeed * 50;   // Apply stronger gravity over time

        // Check if the player lands back on the ground
        if (playerY <= 0.0f) {  // Player has landed back
            playerY = 0.0f;      // Reset to ground level (relative to -0.7f in DrawPlayer)
            isJumping = false;   // Stop jumping
            jumpVelocity = 0.05f; // Reset jump velocity
        }
    }
}

// Function to handle key presses
static void KeyPress(unsigned char key, int x, int y) {
    if (key == ' ' && !isJumping) {  // Space for jump
        isJumping = true;
    }
    if (key == 'd') {  // 'd' for duck, allow ducking in the air
        isDucking = true;
    }
}

// Function to handle collectible collisions
static void CheckCollectibleCollisions() {
    for (auto& collectible : collectibles) {
        if (collectible.active && -0.8f < collectible.x + collectible.size && -0.8f + 0.1f > collectible.x) {
            if (hasMagnet) {
                score += 500;
                collectible.active = false;
                //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\coin.wav");  // Play collect sound effect
                printf("Automatically collected collectible with Magnet! Score: %d\n", score);
                continue;
            }
            if (collectible.y == -0.6f && playerY <= 0.1f) {
                score += 500;
                collectible.active = false;
                //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\coin.wav");  // Play collect sound effect
                printf("Collected ground collectible! Score: %d\n", score);
            }
            else if (playerY >= collectible.y + 0.5f) {
                score += 500;
                collectible.active = false;
                //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\coin.wav");  // Play collect sound effect
                printf("Collected high collectible! Score: %d\n", score);
            }
        }
    }
}

static void CheckPowerUpCollisions() {
    for (auto& powerUp : powerUps) {
        // Check if power-up is active and within the horizontal bounds of the player
        if (powerUp.active && -0.8f < powerUp.x + powerUp.size && -0.8f + 0.1f > powerUp.x) {
            // Check if the player is on the ground or within a certain jumping height
            if (powerUp.y == -0.6f) {  // Assuming playerY = 0 is ground level
                if (playerY <= 0.1f) {
                    // Collect the power-up if the player is on the ground and aligned with it
                    if (powerUp.type == 1) {
                        hasMagnet = true;  // Activate magnet
                        powerUpStartTime = clock();  // Track time when acquired
                        //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\Magnet.wav");  // Play collect sound effect
                        printf("Collected Magnet Power-Up!\n");
                    }
                    else {
                        isInvincible = true;  // Activate invincibility
                        powerUpStartTime = clock();  // Track time   when acquired
                        //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\invincible.wav");  // Play collect sound effect
                        printf("Collected Invincibility Power-Up!\n");
                    }
                    powerUp.active = false;  // Deactivate power-up after it's collected
                }
            }
            else {
                // If the player is jumping, check if they are above the power-up and within range
                if (playerY >= powerUp.y + 0.5f) {
                    // Collect the power-up if the player is above it
                    if (powerUp.type == 1) {
                        hasMagnet = true;  // Activate magnet
                        powerUpStartTime = clock();  // Track time when acquired
                        //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\Magnet.wav");  // Play collect sound effect
                        printf("Collected Magnet Power-Up!\n");
                    }
                    else {
                        isInvincible = true;  // Activate invincibility
                        powerUpStartTime = clock();  // Track time when acquired
                        //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\invincible.wav");  // Play collect sound effect
                        printf("Collected Invincibility Power-Up!\n");
                    }
                    powerUp.active = false;  // Deactivate power-up after it's collected
                }
                else {
                    // Debug information
                    printf("Not collected high collectible: playerY = %.2f, collectible.y = %.2f\n", playerY, powerUp.y);
                }
            }
        }
    }
}

// Function to handle collisions
static void CheckCollisions() {
    for (auto& obstacle : obstacles) {
        if (-0.8f < obstacle.x + obstacle.width && -0.8f + 0.1f > obstacle.x) {
            if (!obstacle.hasHitPlayer) {
                if (obstacle.y == -0.7f && playerY <= 0.0f && !isInvincible) {
                    isKnockedBack = true;
                    knockbackTimer = knockbackDuration;
                    playerX -= knockbackStrength;
                    lives--;
                    //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\obstacle.wav");  // Play hit sound effect
                    printf("Hit ground obstacle! Lives remaining: %d\n", lives);
                    obstacle.hasHitPlayer = true;
                    if (lives == 0) {
                        gameLose = true;  // Set game over flag
                    }
                }
                else if (!isDucking && playerY <= obstacle.height && !isInvincible) {
                    isKnockedBack = true;
                    knockbackTimer = knockbackDuration;
                    playerX -= knockbackStrength;
                    lives--;
                    //playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\obstacle.wav");  // Play hit sound effect
                    printf("Hit above obstacle! Lives remaining: %d\n", lives);
                    obstacle.hasHitPlayer = true;
                    if (lives == 0) {
                        gameLose = true;  // Set game over flag
                    }
                }
            }
        }
        if (obstacle.x + obstacle.width < -0.8f) {
            obstacle.hasHitPlayer = false;
        }
    }
}

// Function to draw the game frame (upper and lower borders)
static void DrawGameFrame() {
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 0.0f);  // Black color

    glBegin(GL_QUADS);
    glVertex2f(-1.0f, 0.9f);  // Left side
    glVertex2f(1.0f, 0.9f);   // Right side
    glVertex2f(1.0f, 1.0f);   // Top right corner
    glVertex2f(-1.0f, 1.0f);  // Top left corner
    glEnd();


    // Lower Border (using quads)
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);  // Bottom left corner
    glVertex2f(1.0f, -1.0f);   // Bottom right corner
    glVertex2f(1.0f, -0.9f);    // Just above the bottom
    glVertex2f(-1.0f, -0.9f);   // Just above the bottom
    glEnd();

    glPopMatrix();
}

// Function to draw the ground
static void DrawGround() {
    glPushMatrix();
    glColor3f(0.5f, 0.35f, 0.05f);  // Brownish color for the ground
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -0.9f);  // Ground above the lower border
    glVertex2f(-1.0f, -0.7f);
    glVertex2f(1.0f, -0.7f);
    glVertex2f(1.0f, -0.9f);
    glEnd();
    glPopMatrix();
}

// Display function
static void Display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameEnd) {
        DisplayGameEnd("Game End"); // Display 'Game End' when time runs out
        return; // Exit early to avoid drawing the game scene
    }

    if (gameLose) {
        DisplayGameEnd("Game Lose"); // Display 'Game Lose' when player loses all lives
        return; // Exit early to avoid drawing the game scene
    }


    // Draw the game frame, health bar, score, time, and obstacles
    DrawGameFrame();
    DrawBackground();
    DrawBoundaries();
    UpdateBackgroundAnimations();
    DrawGround();
    DrawHealthBar();
    DrawScoreAndTime();
    DrawPlayer();
    DrawPowerUps();
    DrawObstacles();  // Add obstacle drawing
    DrawCollectibles();  // Draw all active collectibles
    UpdateAnimations();

    glFlush();
    glutSwapBuffers();
}

// Timer function to handle spawning and movement
static void Timer(int value) {
    // Update game time
    int currentTime = (clock() - startTime) / CLOCKS_PER_SEC;
    gameTime = 60 - currentTime;  // 2-minute countdown

    UpdateBackgroundAnimations(); // Update background animations
    // Start background music
    if (gameTime <= 0) {
        gameEnd = true; // Set game end flag when time runs out
    }

    if (gameEnd || gameLose) {
        glutPostRedisplay(); // Trigger display to show game end/lose screen
        if (gameLose == true) {
            playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\GameEnd.wav");  // Play hit sound effect
        }
        else {
            if (gameEnd == true) {
                playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\GameOver.wav");  // Play hit sound effect
            }
        }

        return; // Exit early to avoid further game logic
    }

    starSpawnCounter++;

    // Adjust this number to control how many frames between star additions
    if (starSpawnCounter > 50) {  // Every 50 frames, add a new star
        Star star;
        star.x = ((rand() % 200) - 100) / 100.0f;  // Random X position between -1 and 1
        star.y = ((rand() % 200) - 100) / 100.0f;  // Random Y position between -1 and 1
        star.size = 0.005f * (rand() % 3 + 1);     // Random size
        stars.push_back(star);                      // Add star to the vector
        starSpawnCounter = 0;                       // Reset counter
    }
    // Update moon's position


    if (knockbackTimer > 0) {
        knockbackTimer--;  // Decrease knockback timer
        playerX -= knockbackStrength;  // Move player back while timer lasts
        if (knockbackTimer == 0) {
            playerX = -0.8f;  // Reset the player to their original X position
        }
    }
    // Knockback and Readjustment Logic
    if (isReadjusting) {
        if (playerX < -0.8f) {
            playerX += readjustSpeed;  // Move player back toward original position
            if (playerX >= -0.8f) {
                playerX = -0.8f;  // Snap back to original position
                isReadjusting = false;  // Stop readjusting
            }
        }
    }
    JumpMechanics();
    MoveObstacles();  // Move the obstacles and collectables
    CheckCollisions();  // Check for collisions
    MoveCollectibles();  // Move all active collectibles
    CheckCollectibleCollisions();  // Check if any collectibles are collected
    SpawnCollectibles();  // Spawn new collectibles periodically
    // Inside the Timer function, right after spawning collectibles
    MovePowerUps();
    SpawnPowerUps();  // Spawn new power-ups periodically
    CheckPowerUpCollisions();


    // Increase game speed every 15 seconds
    if (currentTime % 5 == 0 && currentTime != 0) { // Ensure it doesn't run on the first call
        gameSpeed += 0.0001f;  // Adjust this value for a steady increase
    }

    // Randomly spawn obstacles and collectables every 1-2 seconds
    if (rand() % 50 == 0) { ///or 60
        SpawnObstacles();

    }
    // Check if power-ups should be deactivated
    if (hasMagnet && (clock() - powerUpStartTime) / CLOCKS_PER_SEC >= 5) {
        hasMagnet = false;  // Deactivate magnet
        printf("Magnet Power-Up deactivated.\n");
    }

    if (isInvincible && (clock() - powerUpStartTime) / CLOCKS_PER_SEC >= 5) {
        isInvincible = false;  // Deactivate invincibility
        printf("Invincibility Power-Up deactivated.\n");
    }
    glutPostRedisplay();  // Redraw the screen
    glutTimerFunc(16, Timer, 0);  // Call again after 16 ms (~60 FPS)
}

// Function to handle key releases
static void KeyRelease(unsigned char key, int x, int y) {
    if (key == 'd') {  // Stop ducking
        isDucking = false;
    }
}

// Main function
int main(int argc, char** argv) {
    srand(static_cast<unsigned>(time(0)));  // Seed for random numbers
    playSoundEffect("C:\\Users\\DELL\\Desktop\\OpenGL2DTemplate\\Mice_on_Venus.wav");  // Play hit sound effect

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Marwan's cute 2D Infinite Runner");
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);  // Dark blue background


    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);  // Set the coordinate system
    startTime = clock();  // Record start time
    glutDisplayFunc(Display);
    glutKeyboardFunc(KeyPress);
    glutKeyboardUpFunc(KeyRelease);
    glutTimerFunc(0, Timer, 0);
    glutMainLoop();

    return 0;
}
