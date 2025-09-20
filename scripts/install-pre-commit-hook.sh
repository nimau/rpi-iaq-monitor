#!/usr/bin/env bash
set -e

echo "Installing pre-commit hook to prevent BSEC file commits..."

# Ensure we're in a git repository
if [ ! -d .git ]; then
    echo "❌ Error: Not in a git repository root. Run this from your repository root."
    exit 1
fi

# Create the pre-commit hook
cat > .git/hooks/pre-commit << 'HOOK_EOF'
#!/usr/bin/env bash
set -e

echo "🔍 Checking for proprietary BSEC files..."

# Check staged files for BSEC patterns
blocked=$(git diff --cached --name-only | grep -Ei '(^|/)(bsec/|libbsec|libalgobsec)(/|\.a$|$)' || true)

if [ -n "$blocked" ]; then
    echo "❌ BLOCKED: Attempt to commit proprietary BSEC files detected!"
    echo
    echo "The following files are not allowed to be committed:"
    echo "$blocked" | sed 's/^/  - /'
    echo
    echo "BSEC is proprietary software from Bosch Sensortec and cannot be redistributed."
    echo
    echo "To fix this:"
    echo "1. Remove BSEC files from staging: git reset HEAD <file>"
    echo "2. Add BSEC files to .gitignore if needed"
    echo "3. Ensure users download BSEC themselves (see docs/DEPENDENCIES_BSEC.md)"
    echo
    exit 1
fi

echo "✅ No BSEC files detected in commit. Proceeding..."
HOOK_EOF

# Make the hook executable
chmod +x .git/hooks/pre-commit

echo "✅ Pre-commit hook installed successfully!"
echo
echo "This hook will prevent accidental commits of proprietary BSEC files."
echo "To test it, try: touch bsec/test.txt && git add bsec/test.txt && git commit -m 'test'"
echo
