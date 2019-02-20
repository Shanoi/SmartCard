package com.example.smartcard;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.util.Log;

import com.example.smartcard.statemachine.InitialState;
import com.example.smartcard.statemachine.State;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static com.example.smartcard.HexaValues.APDUOffset.CLA;
import static com.example.smartcard.HexaValues.APDUOffset.INS;
import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.ReturnCode.CLA_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.INS_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_SELECT;

public class cCardService extends HostApduService {

    private byte[] authorizedINS = new byte[]{(byte) 0xa4, (byte) 0xB0, (byte) 0xd6};

    private static final String TAG_APDU = "Command APDU";

    private State currentState;

    private List<State> stateToExecute;

    @Override
    public byte[] processCommandApdu(byte[] commandApdu, Bundle extras) {

        Log.d("Command received", print(commandApdu));

        if (commandApdu[CLA] != 0x00) {
            Log.d(TAG_APDU, "CLA Unknown");
            return CLA_UNKNOWN;

        }

        if (Arrays.asList(authorizedINS).contains(commandApdu[INS])) {

            Log.d(TAG_APDU, "INS Unknown");
            return INS_UNKNOWN;

        }

        if (commandApdu[INS] == 0xa4) {

            return currentState.execute(commandApdu);

        }

        if (commandApdu[INS] == 0xE0) {

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

        stateToExecute = new ArrayList<>();
        currentState = new State(new InitialState());


//        File file = new File(getBaseContext().getCacheDir(), "NDEFFile");
//
//
//        FileOutputStream outputStream;
//
//        try {
//            file.createNewFile();
//            outputStream = new FileOutputStream(file, false);
//            outputStream.close();
//        } catch (Exception e) {
//            e.printStackTrace();
//        }


    }



    private String print(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        sb.append("[ ");
        for (byte b : bytes) {
            sb.append(String.format("0x%02X ", b));
        }
        sb.append("]");
        return sb.toString();
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
// Le NDEF file doit être créé dans la mémoire internet et contiendra les infos que le lecteur récupère
//   --> gestion au onCreate de classe service
//      --> si existe l'utiliser            Dans la mémoire de l'appli
//      --> si n'existe pas le créer
// 6981
// 6986
// 6A82

// MLe / MLc et NDEF