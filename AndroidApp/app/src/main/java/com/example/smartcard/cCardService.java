package com.example.smartcard;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.util.Log;

import com.example.smartcard.statemachine.InitialState;
import com.example.smartcard.statemachine.State;

import java.util.Arrays;

import static com.example.smartcard.HexaValues.APDUOffset.CLA;
import static com.example.smartcard.HexaValues.APDUOffset.INS;
import static com.example.smartcard.HexaValues.ReturnCode.CLA_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.INS_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.hexStringToByteArray;
import static com.example.smartcard.Utility.convertToString;

public class cCardService extends HostApduService {

    private byte[] authorizedINS = new byte[]{(byte) 0xA4, (byte) 0xB0, (byte) 0xD6};

    private static final String TAG_APDU = "Command APDU";

    private State currentState;

    @Override
    public byte[] processCommandApdu(byte[] commandApdu, Bundle extras) {

        Log.d("Command received", convertToString(commandApdu));

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

//          Whenever we encounter the selection command, we reset the state machine
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

//      Initialization of the state. We give a default data to the card, this will be used if there is no file previously stored
        currentState = new State(new InitialState(), getBaseContext(), hexStringToByteArray("003191010A55016170706C652E636F6D110114540266724C612062656C6C6520686973746f697265510008504F4C5954454348"), "NDEFFile");

    }

}