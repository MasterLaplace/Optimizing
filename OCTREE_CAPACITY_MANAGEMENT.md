# DynamicOctree::insert - Capacity Management for Large Items

## Overview

This document describes the enhanced capacity management logic in `DynamicOctree::insert()` that handles scenarios where a large item needs to be inserted but doesn't fit in any sub-node.

## Problem Statement

Previously, when the octree reached capacity and a new item was inserted, the function would attempt to place it in a sub-node. However, if the item was too large to fit in any sub-node (spans multiple sub-nodes), it would simply be added to the current level, potentially exceeding the capacity limit.

## Solution

The enhanced `insert()` function now implements smart capacity management:

1. **Standard Insertion**: First attempts to insert the item into an appropriate sub-node (existing behavior)

2. **Capacity Check**: After the standard insertion attempt, if:
   - The current node has depth > 0 (can have children)
   - The capacity is reached or exceeded (`_pItems.size() >= _CAPACITY`)
   - The new item doesn't fit in any sub-node

3. **Item Rebalancing**: Then:
   - Searches through existing items at the current level
   - Finds the first item that CAN fit in a sub-node
   - Moves that smaller item to the appropriate sub-node
   - Adds the large item to the current level (now with space available)

## Code Example

```cpp
// Scenario: Octree with capacity=4, depth=2
DynamicOctree<Object> octree(BoundaryBox({0,0,0}, {100,100,100}), 4, 2);

// Insert 4 small items (each fits in sub-nodes)
octree.insert(obj1, BoundaryBox({5,5,5}, {10,10,10}));
octree.insert(obj2, BoundaryBox({15,15,15}, {10,10,10}));
octree.insert(obj3, BoundaryBox({25,25,25}, {10,10,10}));
octree.insert(obj4, BoundaryBox({35,35,35}, {10,10,10}));
// Current: 4 items at root level, capacity reached

// Insert a large item that spans multiple sub-nodes
octree.insert(largeObj, BoundaryBox({20,20,20}, {60,60,60}));

// Result:
// - One of the small items (obj1) is moved to a sub-node
// - The large item is added to the root level
// - Capacity is maintained: 4 items at root, 1 item in sub-node
```

## Benefits

1. **Maintains Capacity Limits**: Prevents the root level from growing unbounded
2. **Handles Large Items**: Properly accommodates items that span multiple sub-nodes
3. **Automatic Rebalancing**: Transparently moves items to optimize space usage
4. **Backward Compatible**: Existing behavior preserved for items that fit in sub-nodes

## Technical Details

### Implementation

The logic is added after the main insertion loop in `DynamicOctree::insert()`:

```cpp
// Check if item is too big to fit in any sub-node and capacity is exceeded
if (_DEPTH > 0 && _pItems.size() >= _CAPACITY)
{
    bool itemFitsInSubNode = false;
    for (uint8_t i = 0; i < 8u; ++i)
    {
        if (_rNodes[i].contains(itemsize))
        {
            itemFitsInSubNode = true;
            break;
        }
    }

    // If the new item doesn't fit in any sub-node, try to move an existing item
    if (!itemFitsInSubNode)
    {
        for (auto it = _pItems.begin(); it != _pItems.end(); ++it)
        {
            for (uint8_t i = 0; i < 8u; ++i)
            {
                if (_rNodes[i].contains(it->first))
                {
                    // Move the item to a sub-node
                    auto itemToMove = *it;
                    _pItems.erase(it);
                    
                    if (!_nodes[i])
                        _nodes[i] = std::make_unique<DynamicOctree<OBJ_TYPE>>(_rNodes[i], _CAPACITY, _DEPTH - 1);
                    
                    _nodes[i]->insert(itemToMove.second, itemToMove.first);
                    
                    // Add the large item
                    _pItems.emplace_back(itemsize, item);
                    return {&_pItems, std::prev(_pItems.end())};
                }
            }
        }
    }
}
```

### Performance Considerations

- **Time Complexity**: O(8 + n*8) in worst case where n is the number of items at current level
- **Best Case**: No rebalancing needed if capacity not exceeded or item fits in sub-node
- **Worst Case**: All items at current level need to be checked (still O(n) which is acceptable)

### Edge Cases Handled

1. **Leaf Nodes**: When `_DEPTH == 0`, no rebalancing is attempted
2. **Under Capacity**: When `_pItems.size() < _CAPACITY`, existing behavior preserved
3. **No Suitable Items**: If no existing item can fit in a sub-node, the large item is still added (graceful degradation)
4. **All Items Large**: When all existing items are also too large, capacity may temporarily exceed limit

## Testing

The implementation has been verified with a standalone test program that confirms:
- Normal insertion works correctly (items < capacity)
- Capacity management triggers when appropriate
- Items are correctly moved to sub-nodes
- Large items are properly accommodated
- Total item count is preserved
