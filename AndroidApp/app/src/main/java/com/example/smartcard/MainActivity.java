package com.example.smartcard;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import static com.example.smartcard.Utility.convertToString;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }

    /**
     * This method allows to display the content of the file on screen when the button is pressed
     * @param view
     */
    public void onClick(View view) {

        File file = new File(getBaseContext().getCacheDir(), "NDEFFile");

        TextView textView = findViewById(R.id.textViewContentFile);

        try {

            FileInputStream f = new FileInputStream(file);
            byte[] fileB = new byte[f.available()];

            int index = 0;
            while (f.available() != 0) {

                fileB[index] = (byte) f.read();
                index++;
            }

            int length = (int) ((fileB[0] & 0xFF) << 8) + (int) (fileB[1] & 0xFF) + 2;

            byte[] temp = new byte[length];

            System.arraycopy(fileB, 0, temp, 0, length);

            textView.setText(convertToString(temp));

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }


}
