#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "common.hpp"

// https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
	FRONT, // paralel to the ground
    BACKWARD,
	BACK, // paralel to the ground
    LEFT, // paralel to the ground (they are like this by default)
    RIGHT, // paralel to the ground (they are like this by default)
	UP, // paralel to the ground
	DOWN // paralel to the ground
};

// Default camera values
static const float YAW         = -90.0f;
static const float PITCH       =  0.0f;
static const float SPEED       =  10.0f;
static const float SENSITIVITY =  0.1f;
static const float ZOOM        =  45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

	bool speedup = false;

    // constructor with vectors
	// por agora estes nao sao usados, porque temos o ponto para onde queremos olhar e nao um vetor
    // Camera(glm::vec3 position = glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    // {
    //     Position = position;
    //     WorldUp = up;
    //     Yaw = yaw;
    //     Pitch = pitch;
    //     updateCameraVectors();
    // }
    // // constructor with scalar values
    // Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    // {
    //     Position = glm::vec3(posX, posY, posZ);
    //     WorldUp = glm::vec3(upX, upY, upZ);
    //     Yaw = yaw;
    //     Pitch = pitch;
    //     updateCameraVectors();
    // }

	Camera() {
		// does nothing, should not be used
	}

	Camera(std::istream &file) {
		file.read(reinterpret_cast<char *>(&Position.x), sizeof(Position));
		file.read(reinterpret_cast<char *>(&WorldUp.x), sizeof(WorldUp));
		
		file.read(reinterpret_cast<char *>(&Yaw), sizeof(Yaw));
		file.read(reinterpret_cast<char *>(&Pitch), sizeof(Pitch));

		file.read(reinterpret_cast<char *>(&MovementSpeed), sizeof(MovementSpeed));
		file.read(reinterpret_cast<char *>(&MouseSensitivity), sizeof(MouseSensitivity));
		file.read(reinterpret_cast<char *>(&Zoom), sizeof(Zoom));

		updateCameraVectors();
	}

	void saveTo(std::ofstream &file) const {
		file.write(reinterpret_cast<const char *>(&Position.x), sizeof(Position));
		file.write(reinterpret_cast<const char *>(&WorldUp.x), sizeof(WorldUp));
		
		file.write(reinterpret_cast<const char *>(&Yaw), sizeof(Yaw));
		file.write(reinterpret_cast<const char *>(&Pitch), sizeof(Pitch));

		file.write(reinterpret_cast<const char *>(&MovementSpeed), sizeof(MovementSpeed));
		file.write(reinterpret_cast<const char *>(&MouseSensitivity), sizeof(MouseSensitivity));
		file.write(reinterpret_cast<const char *>(&Zoom), sizeof(Zoom));
	}

	Camera(glm::vec3 position, glm::vec3 lookAtPoint, glm::vec3 up)
	: MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;

		// temos de calcular pitch e yaw manualmente
		glm::vec3 tempVec = glm::normalize(lookAtPoint - position);
		Yaw = glm::degrees(atan2(tempVec.z, tempVec.x));
		Pitch = glm::degrees(atan2(tempVec.y, sqrt((tempVec.z * tempVec.z) + (tempVec.x * tempVec.x))));
		updateCameraVectors();
	}

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, WorldUp);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
		float velocity;
		if (speedup) {
			velocity = 10 * MovementSpeed * deltaTime;
		} else {
        	velocity = MovementSpeed * deltaTime;
		}

		switch(direction) {
			case(FORWARD):
				// normalize this too?????
				Position += Front * velocity;
				break;
			case(BACKWARD):
				// normalize this too?????
				Position -= Front * velocity;
				break;

			case(FRONT):
				// normalize needed since it would get different speeds for different Y values
				// contas manhosas sao para em vez de ir para a frente manter-se no mesmo plano (ex minecraft voar ao carregar no W nunca sobe nem desce)
				Position += glm::normalize(Front * (glm::vec3(1.0f, 1.0f, 1.0f) - WorldUp)) * velocity;
				break;
			case(BACK):
				// normalize needed since it would get different speeds for different Y values
				// contas manhosas sao para em vez de ir para a frente manter-se no mesmo plano (ex minecraft voar ao carregar no W nunca sobe nem desce)
				Position -= glm::normalize(Front * (glm::vec3(1.0f, 1.0f, 1.0f) - WorldUp)) * velocity;
				break;
			case(LEFT):
				Position -= Right * velocity;
				break;
			case(RIGHT):
				Position += Right * velocity;
				break;
			case(UP):
				Position += glm::normalize(Up * WorldUp) * velocity;
				break;
			case(DOWN):
				Position -= glm::normalize(Up * WorldUp) * velocity;
				break;
		}
	}

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

	// update camera vectors???
	void moveTo(const glm::vec3 &pos) {
		this->Position = pos;
	}

	// update camera vectors???
	void move(const glm::vec3 &delta) {
		this->Position += delta;
	}

	// update camera vectors???
	void lookAt(const glm::vec3 &lookAt) {
		// calcular pitch e yaw manualmente
		glm::vec3 tempVec = glm::normalize(lookAt - this->Position);
		Yaw = glm::degrees(atan2(tempVec.z, tempVec.x));
		Pitch = glm::degrees(atan2(tempVec.y, sqrt((tempVec.z * tempVec.z) + (tempVec.x * tempVec.x))));
	}

	// update camera vectors???
	void look(GLfloat deltaPitch, GLfloat deltaYaw) {
		this->Pitch += deltaPitch;
		this->Yaw += deltaYaw;
	}

	void SpeedUp(bool speedup) {
		this->speedup = speedup;
	}

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
