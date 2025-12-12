# Flashing V3S image file to an SD Card

This guide explains how to write the V3S image (`.img`) to an SD card using **Rufus** on Windows.

---

## Prerequisites

- **Rufus** installed  
  - Download: [Rufus Official Website](https://rufus.ie/)  
- A **microSD card** (16gb)
- A **microSD card reader**  
- The `.img` file you want to flash, [V3S Image link](https://mega.nz/file/NsxXyJxJ#MfK2ap5pS7OFOwpEEb7EIrdeT_5X_UcGqff6zBNgyxw)

---

## Steps
- Connect your SD card to your system using a card reader
- Make sure Windows recognizes the card
- Open `Rufus` (no installation required if using the portable version)
- In the `Device` dropdown, choose your SD card
- Click `SELECT` next to *Boot selection*
- Browse and choose your `.img` file
- `Partition scheme`as default
- `File system` as default
- Click `START`
- Rufus will warn that all data on the SD card will be destroyed. Confirm with `OK`
- Once the progress bar is complete and status shows `READY`, you can close Rufus
- Insert the SD card into MCM board & lcd will start processing