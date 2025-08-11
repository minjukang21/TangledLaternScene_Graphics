# Assignment 2 – Tangled-Inspired Floating Lanterns

## Camera Movements (Boat Control)
- Mouse: look around
- **W/S**: move the boat forward/back
- **A/D**: rotate the boat
- **P**: aerial perspective

<figure>
  <img src="assets/pictures/aerial_view.png" alt="aerial view" height="250">
  <figcaption>aerial view by pressing key P</figcaption>
</figure>

## User Interaction (on the boat only)
- **Space**: spawn a lantern
- **C**: lanterns burst from the castle

## Textures Applied
- wooden texture on the boat
- lantern has a sun emblem
- glowing look on the flower

<table>
  <tr>
    <td>
      <figure>
        <img src="assets/pictures/flower_orbit.png" alt="texture of the boat and flower" width="400">
        <figcaption>texture of the boat and flower</figcaption>
      </figure>
    </td>
    <td>
      <figure>
        <img src="assets/pictures/lantern_spawn_texture.jpg" alt="lantern texture" width="400">
        <figcaption>lantern texture</figcaption>
      </figure>
    </td>
  </tr>
</table>

## Dynamic Lighting & Shadow
dynamic lighting applied to the lanterns (light source), and shadows are cast on the boat, island, and castle

<img src="assets/pictures/boat_shadow.png" alt="shadow cast" width="400">
shadow cast by lanterns spawned behind (as the boat moves forward)

<img src="assets/pictures/castle_burst.jpg" alt="castle lit by lantern burst" width="400">
dynamic lights and shadow on the castle as lanterns burst

## Hierarchical Rotation (2-level)
the flower (child) self-rotates and orbits around the boat (parent)
<img src="assets/pictures/flower_rotating.gif" alt="flower rotating around boat" width="640">