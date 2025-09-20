#!/usr/bin/env bash
set -e

echo "🔍 BSEC File Audit - Checking for proprietary files in repository..."
echo

# Check tracked files
echo "Checking tracked files in git repository..."
bsec_files=$(git ls-files | grep -Ei '(^|/)(bsec|libbsec|libalgobsec)(/|\.a$|$)' || true)

if [ -n "$bsec_files" ]; then
    echo "❌ WARNING: Found BSEC-related files tracked in git:"
    echo "$bsec_files" | sed 's/^/  - /'
    echo
    echo "These files should be removed from git tracking:"
    echo "  git rm --cached <file>  # Remove from tracking but keep locally"
    echo "  git rm <file>           # Remove completely"
    echo
    exit_code=1
else
    echo "✅ No BSEC files found in git tracking"
    exit_code=0
fi

echo

# Check working directory for BSEC files
echo "Checking working directory for BSEC files..."
if [ -d "bsec" ]; then
    echo "📁 bsec/ directory exists (ignored by git)"
    if [ -f "bsec/lib/libalgobsec.a" ]; then
        echo "  ✓ Contains libalgobsec.a (properly ignored)"
    fi
else
    echo "📁 No bsec/ directory found"
fi

echo

# Check .gitignore
echo "Checking .gitignore configuration..."
if grep -q "bsec/" .gitignore 2>/dev/null; then
    echo "✅ bsec/ is properly ignored in .gitignore"
else
    echo "⚠️  WARNING: bsec/ not found in .gitignore"
    echo "   Add it with: echo 'bsec/' >> .gitignore"
fi

echo
if [ $exit_code -eq 0 ]; then
    echo "🎉 Audit passed! No BSEC licensing violations detected."
else
    echo "❌ Audit failed! Please fix the issues above."
fi

exit $exit_code
