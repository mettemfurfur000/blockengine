Great question! You're looking to simulate **temperature diffusion** in a 2D grid of static blocks made of different materials. This is a classic **discrete heat diffusion** problem, often solved using **finite difference methods** on a grid. Let's walk through the **required material properties**, the **physics behind diffusion**, and provide **clear pseudocode** for a stable, accurate 4-neighbor iteration.

---

## ✅ 1. Physical Principles: How Temperature Diffuses

Heat diffusion follows **Fourier’s Law**:

\[
q = -k \nabla T
\]

Where:
- \( q \) = heat flux (W/m²)
- \( k \) = thermal conductivity (W/m·K)
- \( \nabla T \) = temperature gradient (K/m)

In discrete form (on a grid), we approximate the heat flow between adjacent blocks using the **discrete Laplacian**.

The resulting temperature update at a cell is:

\[
T_{\text{new}}(i,j) = T_{\text{old}}(i,j) + \Delta t \cdot \frac{k}{\rho c_p} \cdot \left( \frac{T_{\text{neigh}} - T_{\text{self}}}{\text{distance}} \right)
\]

But since we're working on a **grid with equal spacing**, we can simplify this to:

\[
T_{\text{new}}(i,j) = T_{\text{old}}(i,j) + \Delta t \cdot \frac{k}{\rho c_p} \cdot \left( \frac{1}{4} \sum_{\text{neigh}} (T_{\text{neigh}} - T_{\text{self}}) \right)
\]

This is the **discrete heat equation** in 2D for 4-neighbor diffusion.

---

## ✅ 2. Required Material Properties per Block

To model realistic heat transfer between blocks, you need the following **per-material** properties:

| Property | Symbol | Units | Description |
|--------|--------|-------|-------------|
| **Thermal Conductivity** | \( k \) | W/(m·K) | How easily heat flows through the material |
| **Density** | \( \rho \) | kg/m³ | Mass per volume — affects heat capacity |
| **Specific Heat Capacity** | \( c_p \) | J/(kg·K) | How much energy is needed to raise temperature by 1 K |

💡 **Important**:
The **thermal diffusivity** \( \alpha = \frac{k}{\rho c_p} \) is the most important parameter for diffusion rate. It tells you how fast heat spreads through the material.

> 🔍 **Why these?**
> - Without thermal conductivity, heat wouldn’t flow.
> - Without density and specific heat, you can't compute how much energy is stored in the material.
> - Diffusivity determines how fast temperature equilibrates.

> ⚠️ **Note**: For static blocks, we assume **no heat generation**, no convection, and **only conduction** between adjacent blocks.

---

## ✅ 3. Simplifying Assumptions (for simulation)

- Grid is **uniformly spaced** (e.g., 1 meter per cell)
- Time step \( \Delta t \) must be chosen carefully to avoid numerical instability (see stability condition below)
- Only **4 neighbors** (up, down, left, right) matter — no diagonals
- Temperature stored per cell (1 byte initially, 2/4 bytes for precision)
- Use **buffer** to store next temperature values to avoid overwriting during update
- No external heat sources or boundary conditions (unless you add them)

---

## ✅ 4. Pseudocode for 4-Neighbor Diffusion (Stable Iteration)

```pseudocode
// Parameters
const float dt = 0.1;           // Time step (seconds)
const float dx = 1.0;          // Grid spacing (meters), assume 1 m per cell
const float stability_factor = 0.25; // Safety factor to ensure numerical stability

// Grid: 2D array of Temperature values (T[i][j])
// Material properties: array of Material structs, indexed by block

Material materials[material_count];
// Each material has: k, rho, cp

// Initialize grid with initial temperatures
for each cell (i, j):
    T[i][j] = initial_temperature

// Main simulation loop
while simulation_running:
    // Create a buffer for new temperatures
    create_buffer T_new[rows][cols]

    for each cell (i, j):
        // Get current material at (i,j)
        Material mat = materials[block_type(i,j)]

        // Calculate thermal diffusivity
        float alpha = mat.k / (mat.rho * mat.cp)

        // Sum of temperature differences from neighbors (4 directions)
        float temp_diff_sum = 0.0
        float neighbors_sum = 0.0

        // Check up, down, left, right
        directions = [(0,1), (0,-1), (1,0), (-1,0)]  // right, left, down, up

        for (dx, dy) in directions:
            ni = i + dx
            nj = j + dy

            // Check bounds
            if (ni >= 0 && ni < rows && nj >= 0 && nj < cols):
                neighbor_temp = T[ni][nj]
                temp_diff_sum += (neighbor_temp - T[i][j])
                neighbors_sum += 1.0

        // If no neighbors (corner or edge), skip or handle boundary
        if (neighbors_sum == 0):
            T_new[i][j] = T[i][j]  // no change
        else:
            // Apply heat diffusion equation:
            // T_new = T_old + dt * alpha * (1/4) * sum(ΔT)
            float diffusion_term = dt * alpha * (temp_diff_sum / neighbors_sum)
            T_new[i][j] = T[i][j] + diffusion_term

    // Copy buffer to current grid (update)
    for each cell (i, j):
        T[i][j] = T_new[i][j]

    // Optional: Cap temperature to physical bounds (e.g., 0°C to 1000°C)
    for each cell (i, j):
        if T[i][j] < 0.0: T[i][j] = 0.0
        if T[i][j] > 1000.0: T[i][j] = 1000.0

    // Optional: Add time step counter
    time += dt

    // Optional: Stop condition (e.g., max iterations, convergence)
    if (converged): break
```

---

## ✅ 5. Stability Condition (Important!)

The numerical solution is stable only if the time step \( \Delta t \) satisfies the **Courant–Friedrichs–Lewy (CFL) condition** for diffusion:

\[
\Delta t \leq \frac{dx^2}{2 \alpha}
\]

For a grid spacing \( dx = 1 \), and assuming \( \alpha \approx 1 \times 10^{-5} \, \text{m}^2/\text{s} \) (typical for metal or concrete), then:

\[
\Delta t \leq \frac{1}{2 \times 10^{-5}} = 50000 \, \text{seconds}
\]

👉 That’s too large! So in practice:

- Use **small time steps**: e.g., \( \Delta t = 0.01 \) to \( 0.1 \) seconds
- Or use **smaller spatial steps** (e.g., 0.1 m) to allow larger \( \Delta t \)
- Or use **adaptive time-stepping**

💡 Tip: For simulation purposes (e.g., game or demo), you can **scale the time step** to make diffusion faster — just scale all \( \Delta t \) by a factor to simulate "real-world" heat diffusion over time.

---

## ✅ 6. Example Material Values

| Material | \( k \) (W/m·K) | \( \rho \) (kg/m³) | \( c_p \) (J/kg·K) | \( \alpha \) (m²/s) |
|---------|------------------|--------------------|---------------------|------------------------|
| Steel   | 50               | 7800               | 490                 | ~0.0000064             |
| Wood    | 0.15             | 500                | 1700                | ~0.000000001           |
| Concrete| 1.0              | 2400               | 880                 | ~0.00000005            |
| Air     | 0.026            | 1.2                | 1000                | ~0.00000002            |

👉 This shows **steel heats and cools fast**, **wood is slow**, **air is very slow**.

---

## ✅ 7. Extensions You Can Add Later

- **Boundary conditions**: fixed temperature (wall), insulated (no flow), or convective (heat loss to air)
- **Heat sources**: e.g., a fire at a cell (add a heat source term)
- **Non-uniform grid**: variable spacing (more complex)
- **Diagonal neighbors**: for better accuracy (more advanced)
- **Convection or radiation**: if you want to model external environment
- **Phase changes**: melting/freezing (requires extra logic)

---

## ✅ Summary

| Item | What You Need |
|------|----------------|
| **Material properties** | Thermal conductivity \( k \), density \( \rho \), specific heat \( c_p \) |
| **Key derived parameter** | Thermal diffusivity \( \alpha = k / (\rho c_p) \) |
| **Simulation method** | 4-neighbor finite difference with buffer update |
| **Time step** | Small \( \Delta t \), must satisfy CFL condition |
| **Accuracy** | Depends on grid size and time step |

---

## ✅ Final Thought

This approach gives you **realistic, physically grounded temperature diffusion** across a 2D block grid, even with static materials. You can start simple (1 byte temp, basic 4-neighbor) and expand to include phase changes, sources, or better boundary conditions as needed.

Let me know if you'd like:
- A Python version
- A version with visualization
- A version that includes heat sources or convection
- A version with convergence checks

Happy simulating! 🔥📊
