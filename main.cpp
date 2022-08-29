#include <iostream>
#include <string>
#include <cmath>
#include <ctime>
#include <vector>
#include <SFML/Graphics.hpp>
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <typeinfo>

class Game {
public:
    int screenWidth = 1600;
    int screenHeight = 800;

    sf::Font CourierPrime_Regular;

    Game() {
        srand(time(nullptr)); //seed
        CourierPrime_Regular.loadFromFile("../Fonts/CourierPrime-Regular.ttf");
    }
};
Game game;

class DisplayFPS {
public:
    sf::Text text;
    int historySize = 100;
    std::vector <double> historicalFPS;

    DisplayFPS() {
        text.setFont(game.CourierPrime_Regular);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color(255, 255, 255));
        text.setPosition(5, 0);
    }

    void drawFPS(sf::RenderWindow& window, sf::Clock& clock) {
        double fpsThisFrame = getFPS(clock);
        pushAndPop(fpsThisFrame);
        double fpsRollingAverage = returnRollingAverageFPS();
        text.setString("FPS: " + std::to_string(int(fpsRollingAverage)));
        window.draw(text);
    }

    void pushAndPop(double fps) {
        historicalFPS.push_back(fps);
        if (historicalFPS.size() > historySize) {
            historicalFPS.erase(historicalFPS.begin());
        }
    }

    double returnRollingAverageFPS() {
        double sum = 0;
        for (double fps : historicalFPS) {
            sum += fps;
        }
        return sum / historicalFPS.size();
    }

    double getFPS(sf::Clock& clock) {
        sf::Time timeSinceLastFrame = clock.getElapsedTime();
        clock.restart();
        return (1.0 / timeSinceLastFrame.asSeconds());
    }
};
DisplayFPS displayFPS;

class Mouse {
public:
    int x = 0;
    int y = 0;
    int prev_x = 0;
    int prev_y = 0;
    int rel_x = 0;
    int rel_y = 0;
    bool isOverScreen = false;

    void setMouseProperties(sf::Vector2i vect) {
        checkMouseOverScreen(vect);
        setMousePosition(vect);
        getRelativeMouseMovement(vect);
    }

    void checkMouseOverScreen(sf::Vector2i vect) {
        isOverScreen = false;
        if (0 < vect.x && vect.x < game.screenWidth) {
            if (0 < vect.y && vect.y < game.screenHeight) {
                isOverScreen = true;
            }
        }
    }

    void setMousePosition(sf::Vector2i vect) {
        if (0 < vect.x && vect.x < game.screenWidth) {
            x = vect.x;
        }
        if (0 < vect.y && vect.y < game.screenHeight) {
            y = vect.y;
        }
    }

    void getRelativeMouseMovement(sf::Vector2i vect) {
        rel_x = vect.x - prev_x;
        rel_y = vect.y - prev_y;
        prev_x = vect.x;
        prev_y = vect.y;
    }
};
Mouse mouse;

class Vec2 {
public:
    double x = 0.0;
    double y = 0.0;

    Vec2() {}

    void Normalise() {
        double magnitude = sqrt(x * x + y * y);
        x = x / magnitude;
        y = y / magnitude;
    }

    void Scale(double scalar) {
        x = x * scalar;
        y = y * scalar;
    }

    void Zero() {
        x = 0.0;
        y = 0.0;
    }
};

class Particle {
public:
    Vec2 pos;
    Vec2 vel;
    Vec2 acc;
    int id = -1;
    int i = -1;
    int j = -1;

    Particle() {}

};

class Gas {
public:
    int population = 1000;
    double repulsive_force = 0.01;
    int cell_width = 20;
    double interaction_threshold = cell_width;

    int cell_i_max = game.screenWidth / cell_width;
    int cell_j_max = game.screenHeight / cell_width;

    std::vector<sf::Vertex> vertices;
    std::vector<Particle> particles;

    std::vector<std::vector<std::vector<Particle*>>> cells;

    Gas() {}

    void Initialise() {
        for (int i = 0; i < cell_i_max; i++) {
            std::vector<std::vector<Particle*>> column;
            for (int j = 0; j < cell_j_max; j++) {
                std::vector<Particle*> pointers;
                column.push_back(pointers);
            }
            cells.push_back(column);
        }

        for (int i = 0; i < population; i++) {
            sf::Vertex vertex = sf::Vertex(sf::Vector2f(0, 0));
            vertices.push_back(vertex);
        }

        for (int i = 0; i < population; i++) {
            Particle particle;
            particle.id = i;
            double x = (static_cast<double>(rand()) / RAND_MAX) * (game.screenWidth - 100) + 50;
            double y = (static_cast<double>(rand()) / RAND_MAX) * (game.screenHeight - 100) + 50;
            SetPosition(particle, x, y);
            particles.push_back(particle);
        }
    }

    void Respawn(Particle& particle) {
        double x = (static_cast<double>(rand()) / RAND_MAX) * (game.screenWidth - 100) + 50;
        double y = (static_cast<double>(rand()) / RAND_MAX) * (game.screenHeight - 100) + 50;
        particle.vel.Zero();
        particle.acc.Zero();
        SetPosition(particle, x, y);
    }

    void RespawnOffScreen() {
        for (Particle& particle : particles) {
            if (particle.pos.x < 0 || particle.pos.x > game.screenWidth) { Respawn(particle); }
            if (particle.pos.y < 0 || particle.pos.y > game.screenHeight) { Respawn(particle); }
        }
    }

    void Iterate() {
        Partition();
        ClearAcc();
        Repulsion();
        //_LegacyRepulsion();
        Step();
        RespawnOffScreen();
    }

    void Partition() {
        ClearCells();
        for (Particle& particle : particles) {
            int i = static_cast<int>(particle.pos.x) / cell_width;
            int j = static_cast<int>(particle.pos.y) / cell_width;
            particle.i = i;
            particle.j = j;
            std::vector<Particle*>& pointers = cells[i][j];
            pointers.push_back(&particle);
        }
    }

    void ClearCells() {
        for (int i = 0; i < cell_i_max; i++) {
            for (int j = 0; j < cell_j_max; j++) {
                std::vector<Particle*>& pointers = cells[i][j];
                pointers.clear();
            }
        }
    }

    void Repulsion() {
        for (Particle& particle : particles) {
            int i = particle.i;
            int j = particle.j;
            for (Particle* pointer : cells[i][j]) {
                Particle& neighbour = *pointer;
                Repel(particle, neighbour);
            }
            if (j - 1 >= 0) {
                for (Particle* pointer : cells[i][j - 1]) {
                    Particle& neighbour = *pointer;
                    Repel(particle, neighbour);
                }
            }
            if (j + 1 < cell_j_max) {
                for (Particle* pointer : cells[i][j + 1]) {
                    Particle& neighbour = *pointer;
                    Repel(particle, neighbour);
                }
            }
            if (i - 1 >= 0) {
                for (Particle* pointer : cells[i - 1][j]) {
                    Particle& neighbour = *pointer;
                    Repel(particle, neighbour);
                }
                if (j - 1 >= 0) {
                    for (Particle* pointer : cells[i - 1][j - 1]) {
                        Particle& neighbour = *pointer;
                        Repel(particle, neighbour);
                    }
                }
                if (j + 1 < cell_j_max) {
                    for (Particle* pointer : cells[i - 1][j + 1]) {
                        Particle& neighbour = *pointer;
                        Repel(particle, neighbour);
                    }
                }
            }
            if (i + 1 < cell_i_max) {
                for (Particle* pointer : cells[i + 1][j]) {
                    Particle& neighbour = *pointer;
                    Repel(particle, neighbour);
                }
                if (j - 1 >= 0) {
                    for (Particle* pointer : cells[i + 1][j - 1]) {
                        Particle& neighbour = *pointer;
                        Repel(particle, neighbour);
                    }
                }
                if (j + 1 < cell_j_max) {
                    for (Particle* pointer : cells[i + 1][j + 1]) {
                        Particle& neighbour = *pointer;
                        Repel(particle, neighbour);
                    }
                }
            }
        }
    }

    void Repel(Particle& particle, Particle& neighbour) {
        if (particle.id == neighbour.id) { return; }
        double dx = particle.pos.x - neighbour.pos.x;
        double dy = particle.pos.y - neighbour.pos.y;
        double distance = sqrt(dx * dx + dy * dy);
        if (distance > interaction_threshold) { return; }
        Vec2 force;
        force.x = particle.pos.x - neighbour.pos.x;
        force.y = particle.pos.y - neighbour.pos.y;
        force.Normalise();
        double penetration = 1 - (distance / interaction_threshold);
        double strength = penetration * repulsive_force;
        force.Scale(strength);
        particle.acc.x += force.x;
        particle.acc.y += force.y;
    }

    void _LegacyRepulsion() {
        ClearAcc();
        for (Particle& particle : particles) {
            for (Particle& neighbour : particles) {
                if (particle.id == neighbour.id) { continue; }
                double dx = particle.pos.x - neighbour.pos.x;
                double dy = particle.pos.y - neighbour.pos.y;
                double distance = sqrt(dx * dx + dy * dy);
                if (distance > interaction_threshold) { continue; }
                Vec2 force;
                force.x = particle.pos.x - neighbour.pos.x;
                force.y = particle.pos.y - neighbour.pos.y;
                force.Normalise();
                double penetration = 1 - (distance / interaction_threshold);
                double strength = penetration * repulsive_force;
                force.Scale(strength);
                particle.acc.x += force.x;
                particle.acc.y += force.y;
            }
        }
    }

    void ClearAcc() {
        for (Particle& particle : particles) {
            particle.acc.Zero();
        }
    }

    void Step() {
        for (Particle& particle : particles) {
            particle.vel.x += particle.acc.x;
            particle.vel.y += particle.acc.y;
            SetPosition(particle, particle.pos.x + particle.vel.x, particle.pos.y + particle.vel.y);
        }
    }

    void SetPosition(Particle& particle, double x, double y) {
        particle.pos.x = x;
        particle.pos.y = y;
        sf::Vertex& vertex = vertices[particle.id];
        vertex.position.x = static_cast<float>(particle.pos.x);
        vertex.position.y = static_cast<float>(particle.pos.y);
    }

};

int main()
{
    sf::RenderWindow window(sf::VideoMode(game.screenWidth, game.screenHeight), "Hello SFML", sf::Style::Close);
    sf::Clock clock;

    Gas gas;
    gas.Initialise();

    std::vector<std::vector<sf::Text>> text_cords;
    std::vector<std::vector<sf::Text>> text_count;

    for (int i = 0; i < gas.cell_i_max; i++) {
        std::vector<sf::Text> column;
        for (int j = 0; j < gas.cell_j_max; j++) {
            sf::Text text;
            column.push_back(text);
        }
        text_cords.push_back(column);
    }

    for (int i = 0; i < gas.cell_i_max; i++) {
        std::vector<sf::Text> column;
        for (int j = 0; j < gas.cell_j_max; j++) {
            sf::Text text;
            column.push_back(text);
        }
        text_count.push_back(column);
    }

    for (int i = 0; i < gas.cell_i_max; i++) {
        for (int j = 0; j < gas.cell_j_max; j++) {
            sf::Text& text = text_cords[i][j];
            text.setFont(game.CourierPrime_Regular);
            text.setCharacterSize(14);
            text.setFillColor(sf::Color(100, 100, 100));
            text.setPosition(i * gas.cell_width + 25, j * gas.cell_width + 25);
            text.setString(std::to_string(i) + ", " + std::to_string(j));
        }
    }

    for (int i = 0; i < gas.cell_i_max; i++) {
        for (int j = 0; j < gas.cell_j_max; j++) {
            sf::Text& text = text_count[i][j];
            text.setFont(game.CourierPrime_Regular);
            text.setCharacterSize(14);
            text.setFillColor(sf::Color(100, 100, 100));
            text.setPosition(i * gas.cell_width + 25, j * gas.cell_width + 40);
            text.setString(std::to_string(gas.cells[i][j].size()));
        }
    }

    std::vector<sf::Vertex> horizontal;
    std::vector<sf::Vertex> verticle;
    sf::Color gridColor = sf::Color(30, 30, 30);

    for (int i = 0; i < game.screenHeight / gas.cell_width; i++) {
        sf::Vertex left = sf::Vertex(sf::Vector2f(0, i * gas.cell_width), gridColor);
        sf::Vertex right = sf::Vertex(sf::Vector2f(game.screenWidth, i * gas.cell_width), gridColor);
        horizontal.push_back(left);
        horizontal.push_back(right);
    }

    for (int i = 0; i < game.screenWidth / gas.cell_width; i++) {
        sf::Vertex top = sf::Vertex(sf::Vector2f(i * gas.cell_width, 0), gridColor);
        sf::Vertex bottom = sf::Vertex(sf::Vector2f(i * gas.cell_width, game.screenHeight), gridColor);
        verticle.push_back(top);
        verticle.push_back(bottom);
    }

    while (window.isOpen()) {
        mouse.setMouseProperties(sf::Mouse::getPosition(window));

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        gas.Iterate();

        window.clear(sf::Color(0, 0, 0));

        //for (int i = 0; i < gas.cell_i_max; i++) {
        //    for (int j = 0; j < gas.cell_j_max; j++) {
        //        const sf::Text& text = text_cords[i][j];
        //        window.draw(text);
        //    }
        //}
        //for (int i = 0; i < gas.cell_i_max; i++) {
        //    for (int j = 0; j < gas.cell_j_max; j++) {
        //        sf::Text& text = text_count[i][j];
        //        int num = gas.cells[i][j].size();
        //        text.setString(std::to_string(num));
        //    }
        //}
        //for (int i = 0; i < gas.cell_i_max; i++) {
        //    for (int j = 0; j < gas.cell_j_max; j++) {
        //        const sf::Text& text = text_count[i][j];
        //        window.draw(text);
        //    }
        //}

        window.draw(&horizontal[0], horizontal.size(), sf::Lines);
        window.draw(&verticle[0], verticle.size(), sf::Lines);
        window.draw(&gas.vertices[0], gas.vertices.size(), sf::Points);
        displayFPS.drawFPS(window, clock);
        window.display();
    }
    return 0;
}