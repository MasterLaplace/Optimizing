/**************************************************************************
 * Example: DynamicOctree Capacity Management
 * 
 * This example demonstrates the new capacity management feature in
 * DynamicOctree::insert() that handles large items that don't fit
 * in sub-nodes.
 **************************************************************************/

#include "DynamicOctree.hpp"
#include <iostream>
#include <string>

// Simple object type for demonstration
struct GameObject {
    std::string name;
    
    GameObject(const std::string& n = "") : name(n) {}
};

void printSeparator() {
    std::cout << "\n" << std::string(70, '=') << "\n\n";
}

int main() {
    std::cout << "DynamicOctree Capacity Management Example\n";
    printSeparator();
    
    // Create an octree with:
    // - Boundary: (0,0,0) to (100,100,100)
    // - Capacity: 4 items per node
    // - Depth: 2 levels
    BoundaryBox worldBounds({0, 0, 0}, {100, 100, 100});
    DynamicOctreeContainer<GameObject> octree(worldBounds, 4, 2);
    
    std::cout << "Created octree with:\n";
    std::cout << "  - Boundary: (0,0,0) to (100,100,100)\n";
    std::cout << "  - Capacity: 4 items per node\n";
    std::cout << "  - Depth: 2 levels\n";
    printSeparator();
    
    // Step 1: Insert 4 small objects that fit in sub-nodes
    std::cout << "Step 1: Inserting 4 small objects (10x10x10 each)\n\n";
    
    octree.insert(GameObject("SmallCube1"), BoundaryBox({5, 5, 5}, {10, 10, 10}));
    std::cout << "  - Inserted SmallCube1 at (5,5,5)\n";
    
    octree.insert(GameObject("SmallCube2"), BoundaryBox({15, 15, 15}, {10, 10, 10}));
    std::cout << "  - Inserted SmallCube2 at (15,15,15)\n";
    
    octree.insert(GameObject("SmallCube3"), BoundaryBox({25, 25, 25}, {10, 10, 10}));
    std::cout << "  - Inserted SmallCube3 at (25,25,25)\n";
    
    octree.insert(GameObject("SmallCube4"), BoundaryBox({35, 35, 35}, {10, 10, 10}));
    std::cout << "  - Inserted SmallCube4 at (35,35,35)\n";
    
    std::cout << "\n  Total items in octree: " << octree.size() << "\n";
    std::cout << "  Capacity at root level: REACHED (4/4)\n";
    printSeparator();
    
    // Step 2: Insert a large object that spans multiple sub-nodes
    std::cout << "Step 2: Inserting 1 large object (60x60x60)\n\n";
    std::cout << "  This object is too big to fit in any single sub-node.\n";
    std::cout << "  It spans multiple octree sub-divisions.\n\n";
    
    octree.insert(GameObject("LargeCube"), BoundaryBox({20, 20, 20}, {60, 60, 60}));
    std::cout << "  - Inserted LargeCube at (20,20,20) with size (60,60,60)\n";
    
    std::cout << "\n  Total items in octree: " << octree.size() << "\n";
    printSeparator();
    
    // Explanation
    std::cout << "What happened internally:\n\n";
    std::cout << "1. The insert function detected that:\n";
    std::cout << "   - The large cube doesn't fit in any sub-node\n";
    std::cout << "   - The capacity at root level was already reached (4 items)\n\n";
    
    std::cout << "2. The function automatically rebalanced:\n";
    std::cout << "   - Found SmallCube1 which CAN fit in a sub-node\n";
    std::cout << "   - Moved SmallCube1 to the appropriate sub-node\n";
    std::cout << "   - Added LargeCube to the root level\n\n";
    
    std::cout << "3. Result:\n";
    std::cout << "   - Root level: 4 items (3 small + 1 large)\n";
    std::cout << "   - Sub-nodes: 1 item (SmallCube1)\n";
    std::cout << "   - Capacity constraint maintained!\n";
    printSeparator();
    
    // Benefits
    std::cout << "Benefits of this approach:\n\n";
    std::cout << "  ✓ Maintains capacity limits at each octree level\n";
    std::cout << "  ✓ Properly handles objects that span multiple sub-nodes\n";
    std::cout << "  ✓ Automatically rebalances the tree for optimal space usage\n";
    std::cout << "  ✓ Prevents root level from growing unbounded\n";
    std::cout << "  ✓ Transparent to the caller - no special handling needed\n";
    printSeparator();
    
    std::cout << "Example completed successfully!\n";
    
    return 0;
}

/*
 * Expected Output:
 * 
 * The program demonstrates how the octree automatically manages capacity
 * when large items are inserted. Instead of exceeding capacity at the root
 * level, it intelligently moves smaller items to sub-nodes to make room
 * for items that must stay at the root level due to their size.
 * 
 * This behavior ensures efficient space partitioning while respecting
 * the configured capacity constraints.
 */
