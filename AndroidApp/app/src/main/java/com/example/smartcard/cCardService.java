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
import java.util.List;

import static com.example.smartcard.HexaValues.APDUOffset.CLA;
import static com.example.smartcard.HexaValues.APDUOffset.INS;
import static com.example.smartcard.HexaValues.ReturnCode.CLA_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.INS_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.print;

public class cCardService extends HostApduService {

    private byte[] authorizedINS = new byte[]{(byte) 0xa4, (byte) 0xB0, (byte) 0xd6};

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

            Log.d(TAG_APDU, "State Machine STATE");

            return currentState.execute(commandApdu);

        }

        if (commandApdu[INS] == (byte) 0xB0) {

            Log.d(TAG_APDU, "State Machine STATE B0");

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

        currentState = new State(new InitialState());

        File file = new File(getBaseContext().getCacheDir(), "NDEFFile");
        FileOutputStream outputStream;

        Log.d(TAG_APDU, "COUCOU");

        try {
            file.createNewFile();
            outputStream = new FileOutputStream(file, false);
//            003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746f697265510008504F4C5954454348
            outputStream.write(hexStringToByteArray("003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746F697265510008504F4C5954454348065A5FC8CFFE7215A6F393B2113905A1898C849C533054E96D257487946BE6750DBEA971D6A679BDF09B9A5B179D0B4990348A4559D99E9173CEBF2F9B14311E5767B019204F7F66F744A003624612AEDADDD5EDA481A53A7A76C5D8E5BC37825D0FB6C227B3CA0E41A8EBAC68EE5C56E185DB96AA2AABE2C41F1080EC653E2B64B8016A2D5CD0B74851F1546F9763FF2B2E263EB1D2F68BCBC71629360D88D3AE60071378041B5F4EF93CB9B93F69A732922CA3FB7BFC3315701D8D3D728F7CB50952777EAD210899A24261C0E8B450383B334B022347981C186736431A9524BCB158208555286C9F4A490A0A"));
            outputStream.close();
            Log.d(TAG_APDU, "File CREATED");
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {

            FileInputStream f = new FileInputStream(file);
            byte[] fileB = new byte[f.available()];

            int index = 0;
            while (f.available() != 0){

                fileB[index] = (byte) f.read();
                index++;
            }
            Log.d("FILE CONTENT", print(fileB));

            this.currentState.setFile(fileB);

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    private byte[] hexStringToByteArray(String s) {
        byte[] b = new byte[s.length() / 2];
        for (int i = 0; i < b.length; i++) {
            int index = i * 2;
            int v = Integer.parseInt(s.substring(index, index + 2), 16);
            b[i] = (byte) v;
        }
        return b;
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
