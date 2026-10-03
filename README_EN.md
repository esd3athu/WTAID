# WTAID

This is an open-source analog of WTRTI, that was made by AI in one week, as it’s my first release like that.I haven’t had any critical errors so far, 
but it still needs testing. Also keep an eye on RAM usage - I haven’t noticed any leaks in this build and I already fixed some earlier when I found any, but I’m not sure there are none of them left.

### So far only the basic functions are implemented, but the project will gradually improve.

## How to install and run:

### There are 2 ways:

#### 1 - On the top of the main page

  <img width="905" height="207" alt="Снимок экрана 2026-09-27 183424" src="https://github.com/user-attachments/assets/acb688ee-200f-474c-868b-efae12a3a385" />

  <img width="399" height="358" alt="Снимок экрана 2026-09-27 183704" src="https://github.com/user-attachments/assets/506f5dfe-cf01-450c-a1c2-00d4ba27092f" />

#### 2 - In the Releases

  <img width="167" height="84" alt="Снимок экрана 2026-09-29 202243" src="https://github.com/user-attachments/assets/e7140014-6d56-4d20-993e-2c57212a7b92" />

  <img width="1212" height="188" alt="Снимок экрана 2026-09-29 202255" src="https://github.com/user-attachments/assets/7e7b52ce-9e73-4a2e-925c-e530931c8e63" />

## Then unpack it into a folder and click on the highlighted item.

  <img width="697" height="46" alt="Снимок экрана 2026-09-27 183234" src="https://github.com/user-attachments/assets/48f28a12-e2f8-497a-bd97-a1a11d09a457" />

  <img width="505" height="28" alt="Снимок экрана 2026-09-27 183238" src="https://github.com/user-attachments/assets/000d667f-605b-4fdd-bec0-0bea7303073b" />

  <img width="596" height="49" alt="Снимок экрана 2026-09-27 183245" src="https://github.com/user-attachments/assets/67f8c493-9370-4f3f-9467-01b6a65749b9" />

  <img width="592" height="515" alt="Снимок экрана 2026-09-27 183304" src="https://github.com/user-attachments/assets/2f97634b-2eff-4ed1-83e3-29da29b91965" />

## To launch, double-click WTAID.exe.

On the first launch Windows will complain about the app. Allow it to run via 'More info' -> 'Run anyway'. You can also scan it for viruses if you want to - there are none.

You will see a window like this.окно

  <img width="704" height="816" alt="Снимок экрана 2026-09-27 154135" src="https://github.com/user-attachments/assets/c9089e4a-f3b3-491b-a9e6-e651cee78d2e" />

# What each button does:

### 1. <img width="138" height="24" alt="Снимок экрана 2026-09-27 154508" src="https://github.com/user-attachments/assets/0e3c2b91-790e-4445-a7b8-607cb1f38cb8" /> Opens a window like this:

  <img width="334" height="540" alt="Снимок экрана 2026-09-27 154610" src="https://github.com/user-attachments/assets/3ced2ef5-66e1-4b5c-8396-839a0ed52086" />

Clicking the check mark removes/adds the indicator both in this window and in the main GUI.

  <img width="334" height="541" alt="изображение" src="https://github.com/user-attachments/assets/cdb9b3e6-41e7-46ac-abbe-4578be10c7bf" />

  <img width="704" height="670" alt="изображение" src="https://github.com/user-attachments/assets/9d79c721-45f9-431e-90c1-a2e27057b5b3" />

### 2. <img width="87" height="22" alt="изображение" src="https://github.com/user-attachments/assets/4128d674-7886-4cbe-ae7d-d962d5170491" /> Color settings: 

Colors are clickable: 

To save the settings click on <img width="87" height="22" alt="изображение" src="https://github.com/user-attachments/assets/136d1725-d29f-4a8d-a522-2ace3c6f22b6" /> (it appears instead of EDIT), to close - press ESC.

These settings also apply to the overlay.

### 3. <img width="88" height="23" alt="изображение" src="https://github.com/user-attachments/assets/0b5697b6-cec8-445a-b0a2-4250d9eff378" /> / <img width="87" height="22" alt="изображение" src="https://github.com/user-attachments/assets/84705e6e-eae4-413f-86a7-dccbbd86d2b9" /> : toggles overlay visibility

  <img width="222" height="177" alt="изображение" src="https://github.com/user-attachments/assets/d61a6dfe-7168-4934-bace-3edffa6fb5af" /> 

(Example after color customization)
 
  <img width="224" height="178" alt="изображение" src="https://github.com/user-attachments/assets/4fbdb36a-9936-41bb-923f-913d97b51a29" />

### To move the overlay on the screen use `SHIFT + ALT + arrows`

Without selection the shift is applied to all indicators.

Indicators can be selected individually (left-click) or as a group (SHIFT + left-click): :

  <img width="703" height="671" alt="изображение" src="https://github.com/user-attachments/assets/bee610eb-338d-4edc-a11c-89d675d38293" />

  <img width="705" height="672" alt="изображение" src="https://github.com/user-attachments/assets/3039327c-3c71-4954-a31b-bca61d651a91" />

#### After that the shift is applied only to the selected ones.

  <img width="486" height="93" alt="изображение" src="https://github.com/user-attachments/assets/d4f14989-d505-4c11-a487-208e9801f3f4" />

#### Clicking again removes the selection.

# Principles and features of operation
All data is taken from this localhost:

  ```
  http://127.0.0.1:8111/  
  ```

This is the official way to get additional data from the game. WTRTI works the same way. There can be no ban for this, because the program does not interact with the game files or the game-related part of RAM in any way.

Indicators light up in a different color when certain thresholds are reached, for example: speed and G-load - 95% of maximum, fuel time — less than 2 minutes. Later I will add the ability to change these values and sounds when they are reached.

Main "but":

Neither localhost nor the FM DB that I use contain data about the aircraft’s internal max fuel; so drop tanks are also taken into account even if you do not have them (I haven’t really looked for other databases, maybe the data exists somewhere). So FuelPersentFM shows the remaining fuel including them, while FuelPersent shows the remaining fuel relative to what was taken into battle.
