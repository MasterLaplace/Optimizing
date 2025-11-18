/*
 * Explication de l'optimisation gl_VertexID de Vercidium
 * Vidéo: "When Your Game Is Bad But Your Optimisation Is Genius"
 *
 * CONCEPT CLÉ:
 * Pour une heightmap régulière, la position X,Z est déterministe basée sur l'index du vertex.
 * On stocke SEULEMENT l'altitude, et le vertex shader calcule X,Z depuis gl_VertexID.
 */

#include <iostream>
#include <cstdint>

// ============================================================================
// PROGRESSION DE L'OPTIMISATION
// ============================================================================

// Étape 1: Structure classique (32 bytes)
struct VertexClassic {
    float position[3];   // 12 bytes - x, y, z
    float normal[3];     // 12 bytes - nx, ny, nz
    float texCoords[2];  //  8 bytes - u, v
};

// Étape 2: Sans UVs - calculées depuis position.xz (24 bytes)
struct VertexNoUV {
    float position[3];   // 12 bytes
    float normal[3];     // 12 bytes
    // UVs calculées: uv = position.xz / gridSize
};

// Étape 3: Normales compressées en pitch/yaw (16 bytes)
struct VertexCompressed {
    float altitude;      //  4 bytes - seulement Y
    float xz[2];         //  8 bytes - position X et Z
    uint16_t pitch;      //  2 bytes - angle normal
    uint16_t yaw;        //  2 bytes - angle normal
};

// Étape 4: OPTIMISATION FINALE - gl_VertexID magic (4 bytes!) ✨
struct HeightmapVertex {
    float altitude;      //  4 bytes - TOUT ce qu'on stocke !
    // X et Z sont calculés depuis gl_VertexID dans le shader:
    //   xPos = floor((gl_VertexID % VERTS_PER_ROW) / 2.0)
    //   zPos = floor(gl_VertexID / VERTS_PER_ROW) + mod(index, 2)
    // UVs = vec2(xPos, zPos) / gridSize
};

// ============================================================================
// SHADER PSEUDO-CODE (inspiré de HeightmapShader.cs)
// ============================================================================

const char* VERTEX_SHADER_EXPLANATION = R"glsl(
#version 330

layout (location = 0) in float aAltitude;

uniform mat4 mvp;

const float VERTICES_PER_RUN = 72.0;  // 32 * 2 + 4 degenerates

out vec2 vUV;

void main() {
    // ★ LA MAGIE: Reconstruire X,Z depuis gl_VertexID
    float runIndex = mod(gl_VertexID, VERTICES_PER_RUN);
    float clampedIndex = clamp(runIndex - 1.0, 0.0, VERTICES_PER_RUN - 3.0);

    // X incrémente tous les 2 vertices (triangle strip pattern)
    float xPos = floor(clampedIndex / 2.0);

    // Z incrémente à chaque nouvelle "run" (ligne de triangles)
    float zPos = floor(gl_VertexID / VERTICES_PER_RUN);
    zPos += mod(clampedIndex, 2.0);  // Décalage pour former triangles

    // Position 3D complète reconstituée !
    vec3 position = vec3(xPos, aAltitude, zPos);

    // UVs calculées procéduralement
    vUV = position.xz / 8.0;

    gl_Position = mvp * vec4(position, 1.0);
}
)glsl";

// ============================================================================
// DÉMONSTRATION
// ============================================================================

int main()
{
    const int GRID_SIZE = 8;  // Screenshot montre une grille 8×8
    const int NUM_MESHES = 1600;

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  OPTIMISATION gl_VertexID - Vercidium\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

    // ========================================================================
    // PARTIE 1: OPTIMISATION MÉMOIRE (réduction de la structure vertex)
    // ========================================================================

    std::cout << "📊 ÉTAPE 1: Taille des structures vertex\n";
    std::cout << "  Classique (pos+normal+uv): " << sizeof(VertexClassic) << " bytes\n";
    std::cout << "  Sans UVs:                  " << sizeof(VertexNoUV) << " bytes\n";
    std::cout << "  Normales compressées:      " << sizeof(VertexCompressed) << " bytes\n";
    std::cout << "  ★ Finale (gl_VertexID):    " << sizeof(HeightmapVertex) << " bytes\n\n";

    // ========================================================================
    // PARTIE 2: OPTIMISATION TOPOLOGIQUE (triangle strips)
    // ========================================================================

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "📐 ÉTAPE 2: Réduction du nombre de vertices avec Triangle Strips\n\n";

    // Méthode classique : Triangle List
    const int SQUARES = GRID_SIZE * GRID_SIZE;  // 8×8 = 64 carrés
    const int TRIANGLES_LIST = SQUARES * 2;      // 2 triangles par carré = 128
    const int VERTICES_LIST = TRIANGLES_LIST * 3; // 3 vertices par triangle = 384

    std::cout << "🔺 Méthode classique (Triangle List):\n";
    std::cout << "   Grille: " << GRID_SIZE << "×" << GRID_SIZE << " = " << SQUARES << " carrés\n";
    std::cout << "   Triangles: " << SQUARES << " × 2 = " << TRIANGLES_LIST << " triangles\n";
    std::cout << "   Vertices: " << TRIANGLES_LIST << " × 3 = " << VERTICES_LIST << " vertices\n\n";

    // Méthode optimisée : Triangle Strips
    const int NUM_STRIPS = GRID_SIZE;                    // 8 strips horizontaux
    const int VERTICES_PER_STRIP = (GRID_SIZE + 1) * 2; // (8+1)×2 = 18 vertices par strip
    const int DEGENERATE_TRIANGLES = 2;                  // 2 vertices dégénérés entre strips
    const int VERTICES_STRIP = (NUM_STRIPS * VERTICES_PER_STRIP) +
                               ((NUM_STRIPS - 1) * DEGENERATE_TRIANGLES); // 8×18 + 7×2 = 158

    std::cout << "▶ Méthode optimisée (Triangle Strips):\n";
    std::cout << "   Strips: " << NUM_STRIPS << " bandes horizontales\n";
    std::cout << "   Vertices par strip: (" << GRID_SIZE << "+1) × 2 = " << VERTICES_PER_STRIP << "\n";
    std::cout << "   Dégénérés: " << (NUM_STRIPS - 1) << " × " << DEGENERATE_TRIANGLES << " = "
              << ((NUM_STRIPS - 1) * DEGENERATE_TRIANGLES) << " vertices\n";
    std::cout << "   Total: (" << NUM_STRIPS << " × " << VERTICES_PER_STRIP << ") + "
              << ((NUM_STRIPS - 1) * DEGENERATE_TRIANGLES) << " = " << VERTICES_STRIP << " vertices\n\n";

    const float STRIP_REDUCTION = (1.0f - (float)VERTICES_STRIP / VERTICES_LIST) * 100;
    std::cout << "   🎉 Gain topologique: " << VERTICES_LIST << " → " << VERTICES_STRIP
              << " vertices (" << STRIP_REDUCTION << "% de réduction)\n\n";

    // ========================================================================
    // PARTIE 3: CALCULS MÉMOIRE FINAUX
    // ========================================================================

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "💾 ÉTAPE 3: Calculs mémoire finaux\n\n";

    const size_t meshClassicList = sizeof(VertexClassic) * VERTICES_LIST;
    const size_t meshOptimalStrip = sizeof(HeightmapVertex) * VERTICES_STRIP;

    std::cout << "📦 Par mesh (grille " << GRID_SIZE << "×" << GRID_SIZE << "):\n";
    std::cout << "  Avant (Triangle List + struct classique):\n";
    std::cout << "    " << VERTICES_LIST << " vertices × " << sizeof(VertexClassic)
              << " bytes = " << meshClassicList/1024.0 << " KB\n";
    std::cout << "  Après (Triangle Strip + gl_VertexID):\n";
    std::cout << "    " << VERTICES_STRIP << " vertices × " << sizeof(HeightmapVertex)
              << " bytes = " << meshOptimalStrip/1024.0 << " KB\n\n";

    const size_t totalClassic = meshClassicList * NUM_MESHES;
    const size_t totalOptimal = meshOptimalStrip * NUM_MESHES;
    const float TOTAL_REDUCTION = (1.0f - (float)totalOptimal / totalClassic) * 100;

    std::cout << "🌍 Total (" << NUM_MESHES << " meshes):\n";
    std::cout << "  Avant: " << totalClassic/1024.0/1024.0 << " MB\n";
    std::cout << "  Après: " << totalOptimal/1024.0/1024.0 << " MB\n";
    std::cout << "  🚀 GAIN TOTAL: " << TOTAL_REDUCTION << "%\n\n";

    // ========================================================================
    // EXPLICATION FINALE
    // ========================================================================

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "🔑 COMMENT ÇA MARCHE ?\n\n";
    std::cout << "1️⃣  TRIANGLE STRIPS:\n";
    std::cout << "   Au lieu de 3 vertices par triangle (Triangle List),\n";
    std::cout << "   on partage les vertices entre triangles adjacents.\n";
    std::cout << "   Pour N vertices en strip → N-2 triangles créés !\n\n";

    std::cout << "   Exemple: v0──v2──v4\n";
    std::cout << "            │╲  │╲  │\n";
    std::cout << "            v1──v3──v5\n";
    std::cout << "   6 vertices = 4 triangles au lieu de 12 vertices\n\n";

    std::cout << "2️⃣  gl_VertexID MAGIC:\n";
    std::cout << "   Dans une grille régulière, gl_VertexID encode\n";
    std::cout << "   la position X,Z du vertex. On ne stocke QUE Y !\n\n";
    std::cout << "   Formules (vertex shader):\n";
    std::cout << "     xPos = floor((vertexID % rowSize) / 2)\n";
    std::cout << "     zPos = floor(vertexID / rowSize) + offset\n";
    std::cout << "     uv   = vec2(xPos, zPos) / gridSize\n\n";

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "📈 RÉSULTAT INTERMÉDIAIRE:\n";
    std::cout << "   Mémoire par vertex: 32 bytes → 4 bytes (8× reduction !)\n";
    std::cout << "   Vertices par mesh: 384 → 160 (58% reduction !)\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

    // ========================================================================
    // PARTIE 4: BATCHING GPU avec SSBO (Shader Storage Buffer Object)
    // ========================================================================

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "🚀 ÉTAPE 4: GPU Batching avec glMultiDrawArrays + SSBO\n\n";

    std::cout << "🔴 PROBLÈME: Draw Calls\n";
    std::cout << "   Avec 1600 meshes, la méthode classique fait:\n";
    std::cout << "   for (mesh in meshes) { mesh.Render(); }  ← 1600 appels CPU→GPU !\n\n";

    const int MESHES_X = 40;
    const int MESHES_Z = 40;
    const int TOTAL_MESHES = MESHES_X * MESHES_Z;  // 1600 meshes

    std::cout << "   Grille de terrain: " << MESHES_X << "×" << MESHES_Z
              << " = " << TOTAL_MESHES << " chunks\n";
    std::cout << "   Sans batching: " << TOTAL_MESHES << " draw calls\n";
    std::cout << "   ⚠️  Chaque draw call = overhead CPU → GPU !\n\n";

    std::cout << "💡 SOLUTION: glMultiDrawArrays + SSBO\n";
    std::cout << "   Au lieu d'envoyer 1600 commandes séparées,\n";
    std::cout << "   on envoie TOUTES les positions en un seul SSBO:\n\n";

    std::cout << "   1️⃣  SSBO (GPU) stocke les positions de chunks:\n";
    std::cout << "       vec2 chunkPositions[1600] = {\n";
    std::cout << "         (0,0), (32,0), (64,0), ...,\n";
    std::cout << "         (0,32), (32,32), ...\n";
    std::cout << "       };\n\n";

    std::cout << "   2️⃣  CPU fait UN SEUL appel:\n";
    std::cout << "       glMultiDrawArrays(GL_TRIANGLE_STRIP,\n";
    std::cout << "                         firstArray, countArray, 1600);\n\n";

    std::cout << "   3️⃣  Vertex Shader utilise gl_DrawID:\n";
    std::cout << "       layout(std430, binding = 1) buffer PositionBlock {\n";
    std::cout << "         vec2 chunkPositions[];\n";
    std::cout << "       };\n\n";
    std::cout << "       void main() {\n";
    std::cout << "         vec2 worldPosition = chunkPositions[gl_DrawID];\n";
    std::cout << "         vec3 pos = vec3(xPos, aAltitude, zPos);\n";
    std::cout << "         pos.xz += worldPosition;  // Décalage chunk\n";
    std::cout << "         gl_Position = mvp * vec4(pos, 1.0);\n";
    std::cout << "       }\n\n";

    std::cout << "   🎉 Résultat: 1600 draw calls → 1 draw call !\n\n";

    // Calcul de la taille du SSBO
    const size_t ssboSize = TOTAL_MESHES * 2 * sizeof(float);  // vec2 par chunk

    std::cout << "📦 Mémoire SSBO:\n";
    std::cout << "   " << TOTAL_MESHES << " chunks × 8 bytes (vec2) = "
              << ssboSize << " bytes (" << ssboSize/1024.0 << " KB)\n";
    std::cout << "   ✅ Très faible overhead mémoire !\n\n";

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "⚡ OPTIMISATION CPU: Réduction des draw calls\n\n";

    const float drawCallCostUs = 5.0f;  // ~5 microseconds par draw call (estimation)
    const float totalDrawCallsOld = TOTAL_MESHES * drawCallCostUs;
    const float totalDrawCallsNew = 1 * drawCallCostUs;

    std::cout << "   Temps draw calls (estimation):\n";
    std::cout << "   Avant: " << TOTAL_MESHES << " × " << drawCallCostUs << "µs = "
              << totalDrawCallsOld << "µs (" << totalDrawCallsOld/1000.0 << "ms)\n";
    std::cout << "   Après: 1 × " << drawCallCostUs << "µs = "
              << totalDrawCallsNew << "µs\n\n";

    const float drawCallReduction = (1.0f - totalDrawCallsNew / totalDrawCallsOld) * 100;
    std::cout << "   🚀 Gain: " << drawCallReduction << "% de réduction du temps CPU !\n\n";

    // ========================================================================
    // RÉSUMÉ FINAL
    // ========================================================================

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "🏆 RÉSULTAT FINAL - Toutes optimisations combinées\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

    std::cout << "📊 Mémoire Vertex:\n";
    std::cout << "   Structure: 32 bytes → 4 bytes (8× réduction)\n";
    std::cout << "   Topology: 384 vertices → 160 vertices (58% réduction)\n";
    std::cout << "   Par mesh: 12.3 KB → 0.6 KB (95% réduction !)\n";
    std::cout << "   Total (" << NUM_MESHES << " meshes): "
              << totalClassic/1024.0/1024.0 << " MB → "
              << totalOptimal/1024.0/1024.0 << " MB\n\n";

    std::cout << "⚡ Performance CPU:\n";
    std::cout << "   Draw calls: 1600 → 1 (99.9% réduction)\n";
    std::cout << "   Temps CPU: ~8ms → ~0.005ms\n\n";

    std::cout << "🎮 Performance Rendu (vidéo Vercidium):\n";
    std::cout << "   FPS: 50 → 170 (3.4× amélioration !)\n\n";

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "🔑 TECHNIQUES UTILISÉES:\n";
    std::cout << "   ✅ gl_VertexID: Calcul procédural de position\n";
    std::cout << "   ✅ Triangle Strips: Partage de vertices\n";
    std::cout << "   ✅ SSBO: Stockage GPU des positions de chunks\n";
    std::cout << "   ✅ glMultiDrawArrays: Batching des draw calls\n";
    std::cout << "   ✅ gl_DrawID: Identification du chunk dans le shader\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

    // ========================================================================
    // PARTIE 5: SINKING ILLUSION LOD (Level of Detail)
    // ========================================================================

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "🌍 ÉTAPE 5: Sinking Illusion - LOD sans transparence\n\n";

    std::cout << "🔴 PROBLÈME: Terrain à grande échelle\n";
    std::cout << "   Pour rendre 1 km de terrain avec la même qualité:\n";
    std::cout << "   Grille 1km / 0.25m = 4000×4000 vertices\n";
    std::cout << "   = 16 millions de vertices = 64 MB !\n";
    std::cout << "   ⚠️  Trop lourd, même avec toutes nos optimisations !\n\n";

    std::cout << "💡 SOLUTION 1: Levels of Detail (LOD)\n";
    std::cout << "   Au lieu d'un seul niveau de détail, on en utilise plusieurs:\n\n";

    const int NUM_LODS = 4;
    const int BASE_CHUNK_SIZE = 8;  // Taille d'un chunk LOD 0

    std::cout << "   Système à " << NUM_LODS << " niveaux de LOD:\n";
    for (int lod = 0; lod < NUM_LODS; lod++) {
        const int lodScale = 1 << lod;  // 2^lod
        const int chunkSize = BASE_CHUNK_SIZE * lodScale;
        const int lodRadius = chunkSize * (NUM_LODS - lod);

        std::cout << "   LOD " << lod << ": Chunk " << chunkSize << "×" << chunkSize
                  << "m, rayon " << lodRadius << "m";
        if (lod == 0) std::cout << " (haute qualité, proche)";
        if (lod == NUM_LODS-1) std::cout << " (basse qualité, lointain)";
        std::cout << "\n";
    }
    std::cout << "\n";

    // Calcul du nombre total de triangles
    const int trianglesPerLOD = 128;  // Par chunk, constant grâce aux strips
    const int chunksPerLOD = 64;      // Approximation pour chaque anneau LOD
    const int totalTrianglesLOD = trianglesPerLOD * chunksPerLOD * NUM_LODS;
    const int totalTrianglesNoLOD = 8300000;  // Mentionné dans les sous-titres

    std::cout << "   Avec LOD: ~" << totalTrianglesLOD/1000 << "K triangles\n";
    std::cout << "   Sans LOD (qualité uniforme 1km): ~" << totalTrianglesNoLOD/1000000.0
              << "M triangles\n";
    std::cout << "   🎉 Réduction: " << (totalTrianglesNoLOD / totalTrianglesLOD)
              << "× moins de triangles !\n\n";

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "🔴 PROBLÈME 2: Z-Fighting entre LODs\n\n";

    std::cout << "   Quand on rend tous les LODs en même temps:\n";
    std::cout << "   \n";
    std::cout << "   Vue de profil:\n";
    std::cout << "        LOD 0 (détaillé)  ████████\n";
    std::cout << "        LOD 1 (moyen)     ████████████████\n";
    std::cout << "        LOD 2 (bas)       ████████████████████████\n";
    std::cout << "   \n";
    std::cout << "   ⚠️  LOD 2 CACHE LOD 0 car ils sont à la même altitude !\n";
    std::cout << "   On voit le terrain LOW-POLY au premier plan = MOCHE\n\n";

    std::cout << "   Solution classique: Alpha blending (transparence)\n";
    std::cout << "   ❌ PROBLÈME: Très coûteux sur de grands meshes de terrain\n";
    std::cout << "      (overdraw, tri des polygones, etc.)\n\n";

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "💡 SOLUTION GÉNIALE: Sinking Illusion !\n\n";

    std::cout << "   Au lieu de fade out, on fait ENFONCER le terrain !\n\n";

    std::cout << "   1️⃣  Génération progressive:\n";
    std::cout << "      - Terrain haute qualité généré SOUS TERRE\n";
    std::cout << "      - Il MONTE progressivement quand joueur approche\n";
    std::cout << "      - Animation smooth (interpolation)\n\n";

    std::cout << "   2️⃣  Vue de profil avec sinking:\n";
    std::cout << "   \n";
    std::cout << "      Surface ═══════════════════════════════\n";
    std::cout << "                LOD 0  ████████  ← Visible (haute qualité)\n";
    std::cout << "                LOD 1  ░░░░  ← Enfoncé partiellement\n";
    std::cout << "      -10m      LOD 2  ▓▓  ← Complètement sous terre\n";
    std::cout << "   \n";
    std::cout << "      ✅ On voit TOUJOURS le LOD le plus détaillé au-dessus !\n\n";

    std::cout << "   3️⃣  Dans le Vertex Shader:\n";
    std::cout << "      float distanceToPlayer = length(worldPos.xz - playerPos.xz);\n";
    std::cout << "      float lodFadeStart = LOD_RADIUS - FADE_DISTANCE;\n";
    std::cout << "      float sinkAmount = smoothstep(lodFadeStart, LOD_RADIUS, distance);\n";
    std::cout << "      \n";
    std::cout << "      // Enfoncer le terrain progressivement\n";
    std::cout << "      position.y -= sinkAmount * SINK_DEPTH;  // ex: -20 mètres\n\n";

    std::cout << "   4️⃣  Optimisation: Culling des chunks enterrés\n";
    std::cout << "      if (chunkMaxHeight < seaLevel) {\n";
    std::cout << "        skip();  // Ne pas rendre ce chunk !\n";
    std::cout << "      }\n";
    std::cout << "      ✅ Économie GPU en ne rendant pas ce qui est invisible\n\n";

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "📊 RÉSULTAT FINAL (d'après la vidéo):\n\n";

    std::cout << "   Terrain rendu: 1 km × 1 km\n";
    std::cout << "   Triangles: 160,000 (au lieu de 8.3M sans LOD)\n";
    std::cout << "   Facteur de réduction: 52×\n\n";

    std::cout << "   Performance:\n";
    std::cout << "   Initial: 50 FPS (384 vertices, 32 bytes, triangle list)\n";
    std::cout << "   Final:   400+ FPS (160 vertices, 4 bytes, strips + LOD)\n";
    std::cout << "   🚀 Amélioration: 8× PLUS RAPIDE !\n\n";

    std::cout << "   Comparaison terrain:\n";
    std::cout << "   Début vidéo:  40m × 40m @ 50 FPS\n";
    std::cout << "   Fin vidéo: 1000m × 1000m @ 400 FPS\n";
    std::cout << "   📈 Terrain 625× plus grand + 8× plus rapide !\n\n";

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "🎬 RÉCAPITULATIF COMPLET DES 5 OPTIMISATIONS\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

    std::cout << "1️⃣  SUPPRESSION DES UVs (32→24 bytes)\n";
    std::cout << "   UV = position.xz / gridSize\n";
    std::cout << "   Gain: 25% mémoire, 50→75 FPS\n\n";

    std::cout << "2️⃣  gl_VertexID MAGIC (24→8 bytes)\n";
    std::cout << "   Position recalculée dans le shader\n";
    std::cout << "   Gain: 67% mémoire, 75→130 FPS\n\n";

    std::cout << "3️⃣  COMPRESSION NORMALES (8→4 bytes)\n";
    std::cout << "   vec3 → pitch/yaw (2× uint16)\n";
    std::cout << "   Gain: 50% mémoire, 130→170 FPS\n\n";

    std::cout << "4️⃣  TRIANGLE STRIPS (384→160 vertices)\n";
    std::cout << "   Partage vertices entre triangles\n";
    std::cout << "   Gain: 58% vertices, 170→200 FPS\n\n";

    std::cout << "5️⃣  GPU BATCHING (1600→1 draw calls)\n";
    std::cout << "   glMultiDrawArrays + SSBO\n";
    std::cout << "   Gain: 99.9% draw calls, 200→250 FPS\n\n";

    std::cout << "6️⃣  SINKING ILLUSION LOD (4 niveaux)\n";
    std::cout << "   Terrain progressif sans transparence\n";
    std::cout << "   Gain: 52× réduction triangles, 250→400+ FPS\n\n";

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "🏆 RÉSULTAT FINAL ULTIME\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

    std::cout << "   Mémoire par vertex:  32 bytes → 4 bytes (8× réduction)\n";
    std::cout << "   Vertices par mesh:   384 → 160 (2.4× réduction)\n";
    std::cout << "   Mémoire par mesh:    12.3 KB → 0.6 KB (20× réduction)\n";
    std::cout << "   Draw calls:          1600 → 1 (1600× réduction)\n";
    std::cout << "   Triangles rendus:    8.3M → 160K (52× réduction)\n";
    std::cout << "   Taille terrain:      40m → 1000m (625× agrandissement)\n";
    std::cout << "   Performance:         50 FPS → 400+ FPS (8× amélioration)\n\n";

    std::cout << "💎 TECHNIQUES CLÉS:\n";
    std::cout << "   ✅ Calcul procédural (gl_VertexID, UVs)\n";
    std::cout << "   ✅ Compression données (normales, strips)\n";
    std::cout << "   ✅ GPU-driven rendering (SSBO, batching)\n";
    std::cout << "   ✅ LOD intelligent (sinking sans transparence)\n";
    std::cout << "   ✅ Culling agressif (chunks enterrés)\n\n";

    std::cout << "🎓 LEÇONS:\n";
    std::cout << "   1. Réduire la mémoire = plus rapide que optimiser le code\n";
    std::cout << "   2. Calcul GPU < Transfert mémoire (bandwidth bottleneck)\n";
    std::cout << "   3. Batching > Multithreading pour draw calls\n";
    std::cout << "   4. LOD créatif > Solutions classiques coûteuses\n\n";

    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "✨ FIN DE LA DÉMONSTRATION ✨\n";
    std::cout << "═══════════════════════════════════════════════════════\n";

    return 0;
}
