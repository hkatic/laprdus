"""This tool allows generation of gettext .mo compiled files, pot files from source code files
and pot files for merging.

Three new builders are added into the constructed environment:

- gettextMoFile: generates .mo file from .pot file using msgfmt or polib.
- gettextPotFile: Generates .pot file from source code files.
- gettextMergePotFile: Creates a .pot file appropriate for merging into existing .po files.

To properly configure get text, define the following variables:

- gettext_package_bugs_address
- gettext_package_name
- gettext_package_version


"""

from SCons.Action import Action
import subprocess
import shutil


def _compile_mo_with_polib(target, source, env):
	"""Compile .po to .mo using polib (fallback when msgfmt is not available)."""
	try:
		import polib
		po = polib.pofile(str(source[0]))
		po.save_as_mofile(str(target[0]))
		return 0
	except ImportError:
		print("Error: polib is not installed. Install with: pip install polib")
		return 1
	except Exception as e:
		print(f"Error compiling {source[0]}: {e}")
		return 1


def _compile_mo(target, source, env):
	"""Try msgfmt first, fall back to polib if not available."""
	# Check if msgfmt is available
	if shutil.which("msgfmt"):
		try:
			result = subprocess.run(
				["msgfmt", "-o", str(target[0]), str(source[0])],
				capture_output=True,
				text=True
			)
			if result.returncode == 0:
				return 0
		except Exception:
			pass
	# Fall back to polib
	return _compile_mo_with_polib(target, source, env)


def exists(env):
	return True


XGETTEXT_COMMON_ARGS = (
	"--msgid-bugs-address='$gettext_package_bugs_address' "
	"--package-name='$gettext_package_name' "
	"--package-version='$gettext_package_version' "
	"--keyword=pgettext:1c,2 "
	"-c -o $TARGET $SOURCES"
)


def generate(env):
	env.SetDefault(gettext_package_bugs_address="example@example.com")
	env.SetDefault(gettext_package_name="")
	env.SetDefault(gettext_package_version="")

	env["BUILDERS"]["gettextMoFile"] = env.Builder(
		action=Action(_compile_mo, "Compiling translation $SOURCE"),
		suffix=".mo",
		src_suffix=".po",
	)

	env["BUILDERS"]["gettextPotFile"] = env.Builder(
		action=Action("xgettext " + XGETTEXT_COMMON_ARGS, "Generating pot file $TARGET"),
		suffix=".pot",
	)

	env["BUILDERS"]["gettextMergePotFile"] = env.Builder(
		action=Action(
			"xgettext " + "--omit-header --no-location " + XGETTEXT_COMMON_ARGS,
			"Generating pot file $TARGET",
		),
		suffix=".pot",
	)
