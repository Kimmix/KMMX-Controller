import unittest

from patch_hub75 import FUNCTION, PLANE, WEIGHTED_WINDOW, WINDOW_END, WINDOW_START, patch_source


class Hub75PatchTest(unittest.TestCase):
    def test_patch_is_scoped_and_repeatable(self):
        source = f"before\n{FUNCTION}uint8_t brt, int id) {{\n{WINDOW_START}\nold window\n{WINDOW_END}\n{PLANE}\n}}\nafter"
        patched = patch_source(source)
        self.assertIn(WEIGHTED_WINDOW, patched)
        self.assertNotIn("old window", patched)
        self.assertTrue(patched.startswith("before\n"))
        self.assertTrue(patched.endswith("\nafter"))
        self.assertEqual(patch_source(patched), patched)

    def test_unexpected_source_fails(self):
        with self.assertRaises(RuntimeError):
            patch_source(FUNCTION + "uint8_t brt, int id) {\n}\n")


if __name__ == "__main__":
    unittest.main()
