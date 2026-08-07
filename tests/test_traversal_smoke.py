import os, subprocess, sys, tempfile, unittest
from pathlib import Path

REPO = Path("/home/sin/Projects/kingdom-hearts-zero")
BIN = REPO / "build/kh-door"


def _run(inputs):
    env = os.environ.copy()
    env["HOME"] = tempfile.mkdtemp(prefix="khz_test_")
    proc = subprocess.Popen(
        ["timeout", "60s", "stdbuf", "-o0", "-e0", str(BIN), "--terminal"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        env=env, text=True, bufsize=1,
    )
    stdin = proc.stdin
    for ch in inputs:
        if stdin is None:
            break
        stdin.write(ch + "\n")
        stdin.flush()
    if stdin is not None:
        try:
            stdin.close()
        except Exception:
            pass
    try:
        out, _ = proc.communicate(timeout=45)
    except subprocess.TimeoutExpired:
        proc.kill()
        out, _ = proc.communicate(timeout=5)
    return out


class TestKingdomHeartsZero(unittest.TestCase):
    def test_binary_exists(self):
        self.assertTrue(BIN.exists(), f"{BIN} missing")

    def test_traverse_map_valid(self):
        data = (REPO / "data/worlds/traverse_town.json").read_text()
        self.assertIn("S", data)
        self.assertIn("K", data)
        self.assertIn("E", data)

    def test_terminal_menu_renders(self):
        out = _run(["\n"] * 12 + ["q"])
        self.assertIn("DOOR TO DARKNESS", out)
        self.assertIn("quit", out)

    def test_save_engine_ready(self):
        out = _run(["\n"] * 12 + ["q"])
        self.assertIn("[Save]", out)


if __name__ == "__main__":
    unittest.main()
