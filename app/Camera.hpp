#pragma once
#include "emma/Vector2D.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>

// Maps world coordinates (AU, y pointing up like a star chart) to window pixels
// (y pointing down). Kept in double precision so you can zoom from Neptune's
// orbit down to the Moon without the picture shaking
struct Camera {
    static constexpr double MIN_PX_PER_AU = 0.05;
    static constexpr double MAX_PX_PER_AU = 1e9;

    emma::Vector2D center;    // AU, the world point at the middle of the viewport
    double pxPerAU = 200.0;
    sf::FloatRect viewport;   // pixels: the area right of the sidebar, above the status bar

    sf::Vector2f middle() const { return viewport.position + viewport.size * 0.5f; }

    sf::Vector2f toScreen(const emma::Vector2D& w) const {
        const sf::Vector2f m = middle();
        return sf::Vector2f(static_cast<float>(m.x + (w.x - center.x) * pxPerAU),
                            static_cast<float>(m.y - (w.y - center.y) * pxPerAU));
    }

    emma::Vector2D toWorld(sf::Vector2f s) const {
        const sf::Vector2f m = middle();
        return emma::Vector2D(center.x + (s.x - m.x) / pxPerAU, center.y - (s.y - m.y) / pxPerAU);
    }

    // Zoom keeping the world point under screenPoint fixed
    void zoomAt(sf::Vector2f screenPoint, double factor) {
        const emma::Vector2D before = toWorld(screenPoint);
        pxPerAU = std::clamp(pxPerAU * factor, MIN_PX_PER_AU, MAX_PX_PER_AU);
        const emma::Vector2D after = toWorld(screenPoint);
        center += before - after;
    }

    void panPixels(sf::Vector2f delta) {
        center.x -= delta.x / pxPerAU;
        center.y += delta.y / pxPerAU;
    }

    // Center on the origin (the barycenter) with radiusAU visible on the short side
    void fit(double radiusAU) {
        const double half = 0.5 * std::min(viewport.size.x, viewport.size.y);
        pxPerAU = std::clamp(half / radiusAU, MIN_PX_PER_AU, MAX_PX_PER_AU);
        center = emma::Vector2D();
    }
};
