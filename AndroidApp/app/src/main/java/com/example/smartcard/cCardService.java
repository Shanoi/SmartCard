package com.example.smartcard;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.util.Log;

import com.example.smartcard.statemachine.InitialState;
import com.example.smartcard.statemachine.State;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;

import static com.example.smartcard.HexaValues.APDUOffset.CLA;
import static com.example.smartcard.HexaValues.APDUOffset.INS;
import static com.example.smartcard.HexaValues.ReturnCode.CLA_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.INS_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.hexStringToByteArray;
import static com.example.smartcard.Utility.print;

public class cCardService extends HostApduService {

    private byte[] authorizedINS = new byte[]{(byte) 0xA4, (byte) 0xB0, (byte) 0xD6};

    private static final String TAG_APDU = "Command APDU";

    private State currentState;

    @Override
    public byte[] processCommandApdu(byte[] commandApdu, Bundle extras) {

        Log.d("Command received", print(commandApdu));

        if (commandApdu[CLA] != (byte) 0x00) {

            Log.d(TAG_APDU, "CLA Unknown");

            return CLA_UNKNOWN;

        }

        if (Arrays.asList(authorizedINS).contains(commandApdu[INS])) {

            Log.d(TAG_APDU, "INS Unknown");
            return INS_UNKNOWN;

        }

        if (commandApdu[INS] == (byte) 0xA4) {

            Log.d(TAG_APDU, "State Machine STATE A4");

            currentState.setState(new InitialState());
            currentState.getFile().saveFile();

            return currentState.execute(commandApdu);

        }

        if (commandApdu[INS] == (byte) 0xB0) {

            Log.d(TAG_APDU, "State Machine STATE B0");

            return currentState.execute(commandApdu);

        }

        if (commandApdu[INS] == (byte) 0xD6) {

            Log.d(TAG_APDU, "State Machine STATE D6");

            return currentState.execute(commandApdu);

        }

        if (commandApdu[INS] == (byte) 0xE0) {

            Log.d(TAG_APDU, "No compliant state");

            return NO_COMPLIANT_STATE;

        }

        return OK_CODE;
    }

    @Override
    public void onDeactivated(int reason) {
        Log.d("cCardService", "Deactivated: " + reason);
    }

    @Override
    public void onCreate() {

        super.onCreate();

        currentState = new State(new InitialState(), getBaseContext(), hexStringToByteArray("003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746f697265510008504F4C5954454348"), "NDEFFile");

//        currentState.getFile().saveFile();

//        File file = new File(getBaseContext().getCacheDir(), "NDEFFile");
//        FileOutputStream outputStream;
//
//        Log.d(TAG_APDU, "COUCOU");
//
//        try {
//            file.createNewFile();
//            outputStream = new FileOutputStream(file, false);
////            003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746f697265510008504F4C5954454348
//            outputStream.write(hexStringToByteArray("003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746f697265510008504F4C5954454348"));
//            outputStream.close();
//            Log.d(TAG_APDU, "File CREATED");
//        } catch (Exception e) {
//            e.printStackTrace();
//        }
//
//        try {
//
//            FileInputStream f = new FileInputStream(file);
//            byte[] fileB = new byte[f.available()];
//
//            int index = 0;
//            while (f.available() != 0){
//
//                fileB[index] = (byte) f.read();
//                index++;
//            }
//            Log.d("FILE CONTENT", print(fileB));
//
//            this.currentState.setFileContent(fileB);
//
//        } catch (FileNotFoundException e) {
//            e.printStackTrace();
//        } catch (IOException e) {
//            e.printStackTrace();
//        }

    }





//    private byte[] hexStringToByteArray(String s) {
//        int len = s.length();
//        byte[] data = new byte[len / 2];
//        for (int i = 0; i < len; i += 2) {
//            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
//                    + Character.digit(s.charAt(i + 1), 16));
//        }
//        return data;
//    }

}

// Faire une machine d'état

// Créer un CC file en dur et ne plus y toucher
// Le NDEF file doit être créé dans la mémoire interne et contiendra les infos que le lecteur récupère
//   --> gestion au onCreate de classe service
//      --> si existe l'utiliser            Dans la mémoire de l'appli
//      --> si n'existe pas le créer
// 6981
// 6986
// 6A82

// MLe / MLc et NDEF
