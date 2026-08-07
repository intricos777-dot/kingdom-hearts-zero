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
    def test_save_menu_option(self):
        out = _run(["\n"] * 12 + ["2"])
        self.assertIn("save the dark", out.lower())

    def test_load_menu_option(self):
        out = _run(["\n"] * 12 + ["3"])
        self.assertIn("load the dark", out.lower())

    def test_command_deck_renders(self):
        out = _run(["\n"] * 12 + ["1", "\n", "q"])
        self.assertIn("HP", out)
        self.assertIn("MP", out)
        self.assertIn("DECK", out)


if __name__ == "__main__":
    unittest.main()
