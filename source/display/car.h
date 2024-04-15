#ifndef CAR_H
#define CAR_H

#include <vector>

#include "../lib/olcPixelGameEngine.h"
#include "../definitions.h"

class Car {
    const olc::vf2d OFFSET = {256, 256};
    const olc::vf2d START_EAST = {-256, 360};
    const olc::vf2d START_WEST = {1256, 230};
    const std::vector<olc::vf2d> WAIT_TO_GO_EAST1 = {{0, 360}};
    const std::vector<olc::vf2d> WAIT_TO_GO_WEST1 = {{1000, 230}};
    const std::vector<olc::vf2d> WAIT_TO_GO_EAST2 = {{0, 360}}; 
    const std::vector<olc::vf2d> WAIT_TO_GO_WEST2 = {{250, 230}};
    const std::vector<olc::vf2d> PATH_CROSSING_EAST = {{1256, 360}};
    const std::vector<olc::vf2d> PATH_CROSSING_WEST = {{700, 360}, {350, 360}, {200, 230}, {-256, 230}};
    const float DEFAULT_SPEED = 100.0;

    public:
        Car(olc::PixelGameEngine* engine, std::chrono::high_resolution_clock::time_point startTime, std::chrono::duration<double> normalizeStartTimeTo, std::thread::id carId, crossingDatum crossingData);
        ~Car();
        void update(std::chrono::high_resolution_clock::time_point currentTime, float elapsedTime);
        CarState getState();

    private:
        olc::PixelGameEngine* engine;
        std::thread::id carId;
        CarState state;
        crossingDatum crossingData;
        olc::vf2d currPosition;
        olc::Sprite *sprite = nullptr;
        olc::Decal *decal = nullptr;
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::duration<double> normalizeStartTimeTo;
        size_t currentPathIndex;
        std::vector<olc::vf2d> path;
        float speed;

        void updateState(std::chrono::high_resolution_clock::time_point currentTime);
        void initState(CarState state);
        void updatePosition(float elapsedTime);
        void render(olc::vf2d currPosition, float elapsedTime);
        void moveAlongPath(const std::vector<olc::vf2d>& path, float speed, float elapsedTime);
        float calculateCrossingSpeed(const std::vector<olc::vf2d>& path);
};

Car::Car(olc::PixelGameEngine* engine, std::chrono::high_resolution_clock::time_point startTime, std::chrono::duration<double> normalizeStartTimeTo, std::thread::id carId, crossingDatum crossingData) : engine(engine), startTime(startTime), normalizeStartTimeTo(normalizeStartTimeTo), carId(carId), crossingData(crossingData) {
    if(crossingData.direction == Direction::EAST){
        currPosition = START_EAST;
        sprite = new olc::Sprite("./source/assets/vehicles/hatchbackSports_E.png");
    } else {
        currPosition = START_WEST;
        sprite = new olc::Sprite("./source/assets/vehicles/hatchbackSports_W.png");
    }
    currentPathIndex = 0;
    state = CarState::HIDDEN;
    decal = new olc::Decal(sprite);
    speed = DEFAULT_SPEED;
}

Car::~Car() {} // todo clean up

void Car::update(std::chrono::high_resolution_clock::time_point currentTime, float elapsedTime) {
    updateState(currentTime);
    updatePosition(elapsedTime);
}

void Car::updateState(std::chrono::high_resolution_clock::time_point currentTime) {
    std::chrono::duration<double> startWaitTime = crossingData.startWaitTime - normalizeStartTimeTo;
    std::chrono::duration<double> enterTime = crossingData.enterTime - normalizeStartTimeTo;
    std::chrono::duration<double> leaveTime = crossingData.leaveTime - normalizeStartTimeTo;

    auto timeSinceStart = std::chrono::duration<double>(currentTime - startTime);

    if (timeSinceStart > startWaitTime && timeSinceStart < enterTime && state != CarState::WAITING1) { //todo waiting2 state transition?
        std::cout << "waiting" << std::endl;
        state = CarState::WAITING1;
        initState(state);       
    } else if (timeSinceStart >= enterTime && timeSinceStart <= leaveTime && state != CarState::CROSSING) {
        std::cout << "crossing" << std::endl;
        state = CarState::CROSSING;
        initState(state);
    } else if (timeSinceStart > leaveTime && state != CarState::DONE) {
        std::cout << "done" << std::endl;
        state = CarState::DONE;
    }
}

void Car::initState(CarState state) {
    olc::vf2d startPosition;
    std::vector<olc::vf2d> pathFromStart;

    switch(state) {
        case CarState::WAITING1:
            path = (crossingData.direction == Direction::EAST)? WAIT_TO_GO_EAST1 : WAIT_TO_GO_WEST1;
            currentPathIndex = 0;
            break;
        case CarState::WAITING2:
            path = (crossingData.direction == Direction::EAST)? WAIT_TO_GO_EAST2 : WAIT_TO_GO_WEST2;
            currentPathIndex = 0;
            break;
        case CarState::CROSSING:
            path = (crossingData.direction == Direction::EAST)? PATH_CROSSING_EAST : PATH_CROSSING_WEST;
            currentPathIndex = 0;
            pathFromStart = (crossingData.direction == Direction::EAST)? WAIT_TO_GO_EAST1 : WAIT_TO_GO_WEST1;
            pathFromStart.insert(pathFromStart.end(), path.begin(), path.end());
            speed = calculateCrossingSpeed(pathFromStart);
            break;
        default:
            break;
    }
}

void Car::updatePosition(float elapsedTime) {
    switch(state) {
        case CarState::WAITING1:
        case CarState::WAITING2:
        case CarState::CROSSING:
            moveAlongPath(path, speed, elapsedTime);
            render(currPosition, elapsedTime);
            break;
        default:
            break;
    }
}

void Car::render(olc::vf2d currPosition, float elapsedTime) {
    engine->DrawDecal(currPosition - OFFSET, decal);
}

void Car::moveAlongPath(const std::vector<olc::vf2d>& path, float speed, float elapsedTime) {
    if(currentPathIndex >= path.size()) {
        return;
    }

    olc::vf2d distanceToTarget = path[currentPathIndex] - currPosition;
    olc::vf2d distanceToMove = {0.0, 0.0};
    // std::cout << "distanceToTarget " << std::to_string(distanceToTarget.x) << std::to_string(distanceToTarget.y) <<std::endl;

    if(distanceToTarget.mag2() > 0.0) {
        distanceToMove = distanceToTarget.norm() * speed * elapsedTime;
        // std::cout << "distanceToMove " << std::to_string(distanceToMove.x) << std::to_string(distanceToMove.y) << std::endl;
    }

    if(distanceToTarget.mag2() <= distanceToMove.mag2()) {
        distanceToMove = distanceToTarget;
        currentPathIndex++;
    } 

    currPosition += distanceToMove;
}

float Car::calculateCrossingSpeed(const std::vector<olc::vf2d>& path) {
    float totalDistance = 0.0;
    for (size_t i = 1; i < path.size(); i++) {
        olc::vf2d distanceVector = path[i] - path[i - 1];
        totalDistance += distanceVector.mag();
    }
    auto timeDifference = (crossingData.leaveTime) - (crossingData.enterTime);
    double elapsedTimeSeconds = std::chrono::duration<double>(timeDifference).count();
    float averageSpeed = (elapsedTimeSeconds > 0.0) ?  totalDistance / static_cast<float>(elapsedTimeSeconds) : 0.0;
    return averageSpeed;
}

CarState Car::getState() {
    return state;
}

#endif