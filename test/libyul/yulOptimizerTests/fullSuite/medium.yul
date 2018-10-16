{
    function allocate(size) -> p {
        p := mload(0x40)
        mstore(0x40, add(p, size))
    }
    function array_index_access(array, index) -> p {
        p := add(array, mul(index, 0x20))
    }
    pop(allocate(0x20))
    let x := allocate(0x40)
    mstore(array_index_access(x, 3), 2)
}
// ----
// fullSuite
// {
//     {
//         let _1 := 0x20
//         let allocate_p
//         allocate_p := mload(_3)
//         mstore(_3, add(allocate_p, _1))
//         pop(allocate_p)
//         let _3 := 0x40
//         let allocate_p_1
//         allocate_p_1 := mload(_3)
//         mstore(_3, add(allocate_p_1, _3))
//         mstore(add(allocate_p_1, 96), 2)
//     }
// }
