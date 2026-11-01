# ACNH Mobile 0.3.88 · Build 97

- Adopt the official NX iOS cloud-save folder picker (dad38db6), attaching its sheet to the choose button to prevent duplicate presentations from Section rows.
- Remove the native picker replacement while preserving localization, folder bookmarks and the off-by-default behavior.
- Existing data and aoc remain reusable. Picking, cancelling and reopening on iPhone still need device verification.

This and future releases attach only the LiveContainer + SideStore LAN host IPA, without a shell or checksum attachments. LC+SideStore users must allow local network access; the same patched host needs no reinstall. Direct sideloads need no host. Private game IPA and data are not public assets.
